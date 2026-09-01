#!/usr/bin/env python3
# Copyright (c) 2026 The Dimecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Exercise a hybrid PoW/PoS regtest chain through the production staking path."""

import time
from decimal import Decimal

from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import BitcoinTestFramework, REGTEST_GENESIS_TIME
from test_framework.util import (
    assert_equal,
    connect_nodes_bi,
    set_node_times,
    sync_blocks,
    wait_until,
)


FIRST_POS_HEIGHT = 100
TARGET_SPACING = 64
STAKE_MAX_AGE = 30 * 24 * 60 * 60
STAKE_WALL_TIMEOUT = 90
STAKE_VIRTUAL_TIMEOUT = 2 * 60 * 60


class HybridPoSRegtestTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.mocktime = REGTEST_GENESIS_TIME + 60 * 60
        self.extra_args = [
            ["-staking=0"],
            ["-staking=1"],
            ["-staking=0"],
            ["-staking=0"],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

        # Regtest has no masternode data to fetch. Let each isolated node take
        # the no-peer fast path before connecting the test topology.
        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=30,
        )

        for node_index in range(1, self.num_nodes):
            connect_nodes_bi(self.nodes, 0, node_index)
        self.sync_all()

    @staticmethod
    def is_pos_block(block):
        if len(block["tx"]) < 2:
            return False

        coinstake = block["tx"][1]
        return (
            len(coinstake["vin"]) > 0
            and "coinbase" not in coinstake["vin"][0]
            and len(coinstake["vout"]) >= 2
            and coinstake["vout"][0]["value"] == Decimal("0")
            and coinstake["vout"][0]["scriptPubKey"]["hex"] == ""
        )

    def wallet_has_transaction(self, node_index, txid):
        try:
            self.nodes[node_index].gettransaction(txid)
            return True
        except JSONRPCException as error:
            if error.error["code"] != -5:
                raise
            return False

    def set_mocktime(self, timestamp):
        self.mocktime = timestamp
        set_node_times(self.nodes, self.mocktime)

    def restart_with_staking(self, node_index, enabled):
        self.restart_node(
            node_index,
            [
                "-staking={}".format(int(enabled)),
                "-mocktime={}".format(self.mocktime),
            ],
        )
        self.nodes[node_index].setmocktime(self.mocktime)
        wait_until(
            lambda: self.nodes[node_index].mnsync("status")["IsSynced"],
            timeout=30,
        )
        connect_nodes_bi(self.nodes, 0, node_index)
        sync_blocks(self.nodes, wait=0.1, timeout=30)

    def wait_for_pos_block(self, staker_index, start_height):
        next_height = start_height + 1
        deadline = time.time() + STAKE_WALL_TIMEOUT
        virtual_deadline = self.mocktime + STAKE_VIRTUAL_TIMEOUT

        while time.time() < deadline and self.mocktime < virtual_deadline:
            self.set_mocktime(self.mocktime + TARGET_SPACING)
            time.sleep(1)

            staker_height = self.nodes[staker_index].getblockcount()
            if staker_height < next_height:
                continue

            sync_blocks(self.nodes, wait=0.1, timeout=30)
            self.nodes[staker_index].syncwithvalidationinterfacequeue()
            active_height = self.nodes[0].getblockcount()

            for height in range(next_height, active_height + 1):
                block_hash = self.nodes[0].getblockhash(height)
                block = self.nodes[0].getblock(block_hash, 2)
                if not self.is_pos_block(block):
                    continue

                coinstake_txid = block["tx"][1]["txid"]
                if self.wallet_has_transaction(staker_index, coinstake_txid):
                    return block_hash

            next_height = active_height + 1

        raise AssertionError(
            "Node {} did not produce a PoS block within {} wall-clock seconds "
            "and {} virtual seconds".format(
                staker_index, STAKE_WALL_TIMEOUT, STAKE_VIRTUAL_TIMEOUT
            )
        )

    def run_test(self):
        pow_node = self.nodes[0]
        first_staker = self.nodes[1]
        second_staker = self.nodes[2]
        observer = self.nodes[3]

        first_stake_address = first_staker.getnewaddress()
        second_stake_address = second_staker.getnewaddress()

        self.log.info("Funding two independent staking wallets with PoW rewards")
        for height in range(1, FIRST_POS_HEIGHT - 1):
            self.set_mocktime(self.mocktime + TARGET_SPACING)
            destination = first_stake_address if height % 2 else second_stake_address
            pow_node.generatetoaddress(1, destination)

        sync_blocks(self.nodes, wait=0.1, timeout=30)
        for node in self.nodes:
            node.syncwithvalidationinterfacequeue()
            assert_equal(node.getblockcount(), FIRST_POS_HEIGHT - 2)

        self.log.info("Aging the staking outputs")
        self.set_mocktime(self.mocktime + STAKE_MAX_AGE)

        # The 30-day mock-time jump is treated as a long system sleep and
        # restarts masternode synchronization. Wait for that reset and allow
        # the normal regtest quick-sync path to finish again.
        wait_until(
            lambda: all(
                not node.mnsync("status")["IsSynced"] for node in self.nodes
            ),
            timeout=15,
        )
        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=60,
        )

        assert_equal(first_staker.getstakingstatus()["mintablecoins"], True)
        assert_equal(second_staker.getstakingstatus()["mintablecoins"], True)

        self.log.info("Confirming mature stake cannot create a pre-activation block")
        pre_activation_tip = pow_node.getbestblockhash()
        pre_activation_height = pow_node.getblockcount()
        time.sleep(6)
        assert_equal(pow_node.getbestblockhash(), pre_activation_tip)
        assert_equal(pow_node.getblockcount(), pre_activation_height)

        self.log.info("Mining the final pre-activation PoW block")
        self.set_mocktime(self.mocktime + TARGET_SPACING)
        last_pre_activation_hash = pow_node.generatetoaddress(
            1, first_stake_address
        )[0]
        sync_blocks(self.nodes, wait=0.1, timeout=30)
        assert_equal(
            pow_node.getblock(last_pre_activation_hash)["height"],
            FIRST_POS_HEIGHT - 1,
        )

        self.log.info("Mining PoW at activation to bootstrap the first stake modifier")
        self.set_mocktime(self.mocktime + TARGET_SPACING)
        post_activation_pow_hash = pow_node.generatetoaddress(
            1, pow_node.getnewaddress()
        )[0]
        sync_blocks(self.nodes, wait=0.1, timeout=30)
        post_activation_pow = observer.getblock(post_activation_pow_hash, 2)
        assert_equal(post_activation_pow["height"], FIRST_POS_HEIGHT)
        assert_equal(self.is_pos_block(post_activation_pow), False)
        assert_equal(post_activation_pow["confirmations"] > 0, True)

        self.log.info("Producing the first PoS block with wallet one")
        first_pos_hash = self.wait_for_pos_block(1, FIRST_POS_HEIGHT)
        first_pos_block = observer.getblock(first_pos_hash, 2)
        assert_equal(first_pos_block["height"] > FIRST_POS_HEIGHT, True)
        assert_equal(self.is_pos_block(first_pos_block), True)
        first_coinstake_txid = first_pos_block["tx"][1]["txid"]
        assert_equal(self.wallet_has_transaction(1, first_coinstake_txid), True)
        assert_equal(self.wallet_has_transaction(2, first_coinstake_txid), False)

        self.log.info("Switching production staking to wallet two")
        second_stake_start_height = pow_node.getblockcount()
        self.restart_with_staking(1, False)
        self.restart_with_staking(2, True)
        second_pos_hash = self.wait_for_pos_block(2, second_stake_start_height)
        second_pos_block = observer.getblock(second_pos_hash, 2)
        assert_equal(self.is_pos_block(second_pos_block), True)
        second_coinstake_txid = second_pos_block["tx"][1]["txid"]
        assert_equal(self.wallet_has_transaction(1, second_coinstake_txid), False)
        assert_equal(self.wallet_has_transaction(2, second_coinstake_txid), True)

        self.log.info("Stopping staking and verifying every node's active chain")
        self.restart_with_staking(2, False)
        sync_blocks(self.nodes, wait=0.1, timeout=30)
        best_hash = pow_node.getbestblockhash()

        for node in self.nodes:
            assert_equal(node.getbestblockhash(), best_hash)
            assert_equal(node.verifychain(4, 0), True)

        self.log.info("Stopping every node for a disconnected persistence check")
        restart_args = [
            [
                "-staking=0",
                "-connect=0",
                "-mocktime={}".format(self.mocktime),
            ]
            for _ in self.nodes
        ]

        self.stop_nodes()
        self.start_nodes(restart_args)
        set_node_times(self.nodes, self.mocktime)

        self.log.info("Verifying each saved chain before reconnecting the nodes")
        for node in self.nodes:
            assert_equal(node.getconnectioncount(), 0)
            node.syncwithvalidationinterfacequeue()
            assert_equal(node.getbestblockhash(), best_hash)
            assert_equal(
                node.getblock(first_pos_hash, 2)["confirmations"] > 0,
                True,
            )
            assert_equal(
                node.getblock(second_pos_hash, 2)["confirmations"] > 0,
                True,
            )
            assert_equal(node.verifychain(4, 0), True)

        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=30,
        )

        self.log.info("Reconnecting the nodes and confirming the preserved chain")
        for node_index in range(1, self.num_nodes):
            connect_nodes_bi(self.nodes, 0, node_index)

        sync_blocks(self.nodes, wait=0.1, timeout=30)

        for node in self.nodes:
            assert_equal(node.getbestblockhash(), best_hash)
            assert_equal(node.verifychain(4, 0), True)


if __name__ == "__main__":
    HybridPoSRegtestTest().main()
