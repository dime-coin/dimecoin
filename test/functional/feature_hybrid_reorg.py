#!/usr/bin/env python3
# Copyright (c) 2026 The Dimecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Exercise mixed PoW/PoS work selection, reorganization, and persistence.

This test deliberately uses only the regtest production controls and existing
node behavior. It does not construct blocks, bypass staking requirements, or
introduce alternate validation rules.
"""

from test_framework.hybrid import (
    HybridChainController,
    HybridRunRecorder,
    PROOF_OF_STAKE,
    PROOF_OF_WORK,
    is_pos_block,
)
from test_framework.test_framework import BitcoinTestFramework, REGTEST_GENESIS_TIME
from test_framework.util import (
    assert_equal,
    connect_nodes_bi,
    disconnect_nodes,
    set_node_times,
    sync_blocks,
    wait_until,
)


FIRST_POS_HEIGHT = 100
TARGET_SPACING = 64
STAKE_MAX_AGE = 30 * 24 * 60 * 60
POS_ATTEMPTS = 120


class HybridReorgTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.mocktime = REGTEST_GENESIS_TIME + 60 * 60
        self.extra_args = [["-staking=0"] for unused in range(self.num_nodes)]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()
        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=30,
        )
        for node_index in range(1, self.num_nodes):
            connect_nodes_bi(self.nodes, 0, node_index)
        self.sync_all()

    def set_mocktime(self, timestamp):
        self.mocktime = timestamp
        set_node_times(self.nodes, timestamp)

    def sync_node_group(self, node_indexes):
        group = [self.nodes[node_index] for node_index in node_indexes]
        sync_blocks(group, wait=0.1, timeout=30)
        for node in group:
            node.syncwithvalidationinterfacequeue()

    def wait_for_masternode_resync(self):
        wait_until(
            lambda: all(not node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=15,
        )
        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=60,
        )

    def bootstrap_hybrid_chain(self):
        pow_node = self.nodes[0]
        stake_addresses = {
            1: self.nodes[1].getnewaddress(),
            2: self.nodes[2].getnewaddress(),
        }

        self.log.info("Funding two independent staking branches with common PoW history")
        for height in range(1, FIRST_POS_HEIGHT - 1):
            self.set_mocktime(self.mocktime + TARGET_SPACING)
            staker_index = 1 if height % 2 else 2
            pow_node.generatetoaddress(1, stake_addresses[staker_index])

        self.sync_node_group(range(self.num_nodes))
        assert_equal(pow_node.getblockcount(), FIRST_POS_HEIGHT - 2)

        self.log.info("Aging the common staking outputs")
        self.set_mocktime(self.mocktime + STAKE_MAX_AGE)
        self.wait_for_masternode_resync()

        self.log.info("Mining PoW through the configured PoS activation height")
        for unused in range(2):
            self.set_mocktime(self.mocktime + TARGET_SPACING)
            pow_node.generatetoaddress(1, pow_node.getnewaddress())

        self.sync_node_group(range(self.num_nodes))
        assert_equal(pow_node.getblockcount(), FIRST_POS_HEIGHT)
        assert_equal(self.nodes[1].getstakingstatus()["mintablecoins"], True)
        assert_equal(self.nodes[2].getstakingstatus()["mintablecoins"], True)

        activation_hash = pow_node.getblockhash(FIRST_POS_HEIGHT)
        activation_block = pow_node.getblock(activation_hash, 2)
        assert_equal(is_pos_block(activation_block), False)
        return activation_block

    def split_network(self):
        self.log.info(
            "Splitting the network into branch A (nodes 0/1) and "
            "branch B (nodes 2/3)"
        )
        disconnect_nodes(self.nodes[0], 2)
        disconnect_nodes(self.nodes[2], 0)
        disconnect_nodes(self.nodes[0], 3)
        disconnect_nodes(self.nodes[3], 0)
        connect_nodes_bi(self.nodes, 2, 3)
        self.sync_node_group((0, 1))
        self.sync_node_group((2, 3))

    def make_controller(self, name, observer_index, node_indexes):
        recorder = HybridRunRecorder(
            "scripted",
            None,
            None,
            None,
            [],
        )

        def sync_branch():
            self.sync_node_group(node_indexes)

        controller = HybridChainController(
            self.nodes,
            observer_index,
            self.set_mocktime,
            sync_branch,
            recorder,
            self.log,
            POS_ATTEMPTS,
            TARGET_SPACING,
            "feature_hybrid_reorg.py",
        )
        controller.set_time(self.mocktime)
        self.log.info("Initialized controlled producer for %s", name)
        return controller

    def assert_proof_and_bits(self, node, record, expected_proof, expected_bits):
        block = node.getblock(record["hash"], 2)
        actual_proof = PROOF_OF_STAKE if is_pos_block(block) else PROOF_OF_WORK
        assert_equal(actual_proof, expected_proof)
        assert_equal(block["bits"], expected_bits)
        return block

    def assert_active_branch(self, node, expected_tip, branches):
        assert_equal(node.getbestblockhash(), expected_tip)
        assert_equal(node.getblockcount(), branches["branch_b_tip"]["height"])
        assert_equal(
            node.getblock(branches["branch_b_pos_hash"], 2)["confirmations"] > 0,
            True,
        )
        if node.gettxout(
            branches["branch_b_coinstake_outpoint"]["txid"],
            branches["branch_b_coinstake_outpoint"]["vout"],
            False,
        ) is None:
            raise AssertionError("The active branch B coinstake output is missing")
        assert_equal(node.verifychain(4, 0), True)

    def assert_orphaned_branch(self, node_index, branches):
        node = self.nodes[node_index]
        assert_equal(
            node.getblock(branches["branch_a_pos_hash"], 2)["confirmations"],
            -1,
        )
        assert_equal(
            node.getblock(branches["branch_a_tip"]["hash"], 2)["confirmations"],
            -1,
        )
        assert_equal(
            node.gettxout(
                branches["branch_a_coinstake_outpoint"]["txid"],
                branches["branch_a_coinstake_outpoint"]["vout"],
                False,
            ),
            None,
        )

    def build_competing_branches(self, pow_bits):
        branch_a = self.make_controller("branch A", 0, (0, 1))
        branch_a_pos = branch_a.produce_pos(1)
        branch_a_pow_one = branch_a.produce_pow(0)
        branch_a_pow_two = branch_a.produce_pow(0)

        branch_a_pos_block = self.nodes[0].getblock(branch_a_pos["hash"], 2)
        pos_bits = branch_a_pos_block["bits"]
        if pos_bits == pow_bits:
            raise AssertionError(
                "Regtest PoW and PoS targets unexpectedly use identical nBits"
            )

        self.assert_proof_and_bits(
            self.nodes[0], branch_a_pos, PROOF_OF_STAKE, pos_bits
        )
        self.assert_proof_and_bits(
            self.nodes[0], branch_a_pow_one, PROOF_OF_WORK, pow_bits
        )
        self.assert_proof_and_bits(
            self.nodes[0], branch_a_pow_two, PROOF_OF_WORK, pow_bits
        )

        branch_a_coinstake_outpoint = {
            "txid": branch_a_pos_block["tx"][1]["txid"],
            "vout": 1,
        }
        if self.nodes[0].gettxout(
            branch_a_coinstake_outpoint["txid"],
            branch_a_coinstake_outpoint["vout"],
            False,
        ) is None:
            raise AssertionError("The active branch A coinstake output is missing")

        branch_b = self.make_controller("branch B", 3, (2, 3))
        branch_b_pos_one = branch_b.produce_pos(2)
        branch_b_pos_two = branch_b.produce_pos(2)
        branch_b_pos_one_block = self.assert_proof_and_bits(
            self.nodes[3], branch_b_pos_one, PROOF_OF_STAKE, pos_bits
        )
        self.assert_proof_and_bits(
            self.nodes[3], branch_b_pos_two, PROOF_OF_STAKE, pos_bits
        )

        branch_a_tip = self.nodes[0].getblock(branch_a_pow_two["hash"])
        branch_b_tip = self.nodes[3].getblock(branch_b_pos_two["hash"])
        if branch_b_tip["height"] >= branch_a_tip["height"]:
            raise AssertionError(
                "Branch B must be shorter to exercise work-based selection"
            )
        if int(branch_b_tip["chainwork"], 16) <= int(branch_a_tip["chainwork"], 16):
            raise AssertionError(
                "The shorter PoS-heavy branch does not have more chainwork"
            )

        branch_b_coinstake_outpoint = {
            "txid": branch_b_pos_one_block["tx"][1]["txid"],
            "vout": 1,
        }
        if self.nodes[3].gettxout(
            branch_b_coinstake_outpoint["txid"],
            branch_b_coinstake_outpoint["vout"],
            False,
        ) is None:
            raise AssertionError("The active branch B coinstake output is missing")

        return {
            "branch_a_tip": branch_a_tip,
            "branch_a_pos_hash": branch_a_pos["hash"],
            "branch_a_coinstake_outpoint": branch_a_coinstake_outpoint,
            "branch_b_tip": branch_b_tip,
            "branch_b_pos_hash": branch_b_pos_one["hash"],
            "branch_b_coinstake_outpoint": branch_b_coinstake_outpoint,
        }

    def reconnect_and_verify_reorg(self, branches):
        self.log.info(
            "Reconnecting the branches and selecting the shorter "
            "higher-work history"
        )
        connect_nodes_bi(self.nodes, 0, 2)
        self.sync_node_group(range(self.num_nodes))

        expected_tip = branches["branch_b_tip"]["hash"]
        for node in self.nodes:
            self.assert_active_branch(node, expected_tip, branches)

        for node_index in (0, 1):
            self.assert_orphaned_branch(node_index, branches)

        return expected_tip

    def verify_restart_persistence(self, expected_tip, branches):
        self.log.info(
            "Restarting every node disconnected and verifying saved mixed-chain state"
        )
        restart_args = [
            [
                "-staking=0",
                "-connect=0",
                "-mocktime={}".format(self.mocktime),
            ]
            for unused in self.nodes
        ]

        self.stop_nodes()
        self.start_nodes(restart_args)
        set_node_times(self.nodes, self.mocktime)

        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=30,
        )
        for node in self.nodes:
            assert_equal(node.getconnectioncount(), 0)
            node.syncwithvalidationinterfacequeue()
            self.assert_active_branch(node, expected_tip, branches)

        for node_index in (0, 1):
            self.assert_orphaned_branch(node_index, branches)

        self.log.info("Reconnecting the restarted nodes")
        for node_index in range(1, self.num_nodes):
            connect_nodes_bi(self.nodes, 0, node_index)
        self.sync_node_group(range(self.num_nodes))

        for node in self.nodes:
            self.assert_active_branch(node, expected_tip, branches)

    def run_test(self):
        activation_block = self.bootstrap_hybrid_chain()
        pow_bits = activation_block["bits"]

        self.split_network()
        branches = self.build_competing_branches(pow_bits)
        expected_tip = self.reconnect_and_verify_reorg(branches)
        self.verify_restart_persistence(expected_tip, branches)


if __name__ == "__main__":
    HybridReorgTest().main()
