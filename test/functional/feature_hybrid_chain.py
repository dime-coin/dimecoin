#!/usr/bin/env python3
# Copyright (c) 2026 The Dimecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Produce controlled, seeded-random, and live hybrid PoW/PoS regtest chains."""

import json
import os
import random
import shlex
import time

from test_framework.hybrid import (
    PROOF_OF_STAKE,
    HybridChainController,
    HybridRunRecorder,
    HybridSchedule,
    is_pos_block,
    parse_node_indexes,
)
from test_framework.test_framework import BitcoinTestFramework, REGTEST_GENESIS_TIME
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    connect_nodes_bi,
    set_node_times,
    sync_blocks,
    wait_until,
)


FIRST_POS_HEIGHT = 100
TARGET_SPACING = 64
STAKE_MAX_AGE = 30 * 24 * 60 * 60
OBSERVER_INDEX = 3
MAX_PRODUCER_INDEX = OBSERVER_INDEX - 1
LIVE_WALL_TIMEOUT = 120
LIVE_VIRTUAL_TIMEOUT = 3 * 60 * 60


class HybridChainTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 4
        self.mocktime = REGTEST_GENESIS_TIME + 60 * 60
        self.extra_args = [["-staking=0"] for unused in range(self.num_nodes)]

    def add_options(self, parser):
        parser.add_argument(
            "--mode",
            choices=("live", "random", "scripted"),
            default="random",
            help="Hybrid block production mode (default: random)",
        )
        parser.add_argument("--blocks", type=int, default=10, help="Blocks to produce in random or live mode")
        parser.add_argument("--seed", type=int, default=42891, help="Seed for random and live scheduling")
        parser.add_argument("--pow-weight", type=int, default=50, help="Relative PoW production weight")
        parser.add_argument("--pos-weight", type=int, default=50, help="Relative PoS production weight")
        parser.add_argument("--pow-nodes", default="0", help="Comma-separated controlled PoW producer indexes")
        parser.add_argument("--pos-nodes", default="1,2", help="Comma-separated controlled PoS producer indexes")
        parser.add_argument("--sequence", default="", help="Scripted proof:node sequence")
        parser.add_argument(
            "--pos-attempts",
            type=int,
            default=120,
            help="Maximum mock-time searches for each requested PoS block",
        )
        parser.add_argument(
            "--record-file",
            default="",
            help="Optional path for the JSON run record",
        )

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

    def sync_hybrid_chain(self):
        sync_blocks(self.nodes, wait=0.1, timeout=30)
        for node in self.nodes:
            node.syncwithvalidationinterfacequeue()

    def validate_producers(self, indexes, option_name):
        for index in indexes:
            if index > MAX_PRODUCER_INDEX:
                raise ValueError(
                    "{} names node {}, but producer indexes must be between 0 and {} so node {} remains an observer".format(
                        option_name,
                        index,
                        MAX_PRODUCER_INDEX,
                        OBSERVER_INDEX,
                    )
                )

    def replay_command(self):
        command = [
            os.path.basename(__file__),
            "--mode={}".format(self.options.mode),
            "--blocks={}".format(self.options.blocks),
            "--seed={}".format(self.options.seed),
            "--pow-weight={}".format(self.options.pow_weight),
            "--pos-weight={}".format(self.options.pos_weight),
            "--pow-nodes={}".format(self.options.pow_nodes),
            "--pos-nodes={}".format(self.options.pos_nodes),
            "--pos-attempts={}".format(self.options.pos_attempts),
        ]
        if self.options.mode == "scripted":
            command.append("--sequence={}".format(self.options.sequence))
        return " ".join(shlex.quote(argument) for argument in command)

    def wait_for_masternode_resync(self):
        wait_until(
            lambda: all(not node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=15,
        )
        wait_until(
            lambda: all(node.mnsync("status")["IsSynced"] for node in self.nodes),
            timeout=60,
        )

    def bootstrap_chain(self, staking_nodes):
        pow_node = self.nodes[0]
        if staking_nodes:
            destinations = {
                node_index: self.nodes[node_index].getnewaddress()
                for node_index in staking_nodes
            }
        else:
            destinations = {0: pow_node.getnewaddress()}

        self.log.info("Bootstrapping the hybrid chain with PoW")
        ordered_stakers = sorted(destinations)
        for height in range(1, FIRST_POS_HEIGHT - 1):
            self.set_mocktime(self.mocktime + TARGET_SPACING)
            destination_node = ordered_stakers[(height - 1) % len(ordered_stakers)]
            pow_node.generatetoaddress(1, destinations[destination_node])

        self.sync_hybrid_chain()
        assert_equal(pow_node.getblockcount(), FIRST_POS_HEIGHT - 2)
        assert_equal(pow_node.generatepos(0), [])
        assert_raises_rpc_error(
            -8,
            "nblocks must not be negative",
            pow_node.generatepos,
            -1,
        )
        assert_raises_rpc_error(
            -8,
            "maxtries must be greater than zero",
            pow_node.generatepos,
            1,
            0,
        )
        assert_raises_rpc_error(
            -1,
            "Proof of stake is not active until height {}".format(FIRST_POS_HEIGHT),
            self.nodes[staking_nodes[0] if staking_nodes else 0].generatepos,
            1,
            1,
        )

        if staking_nodes:
            self.log.info("Aging controlled staking outputs")
            self.set_mocktime(self.mocktime + STAKE_MAX_AGE)
            self.wait_for_masternode_resync()

        self.log.info("Mining the final pre-activation and activation PoW blocks")
        for unused in range(2):
            self.set_mocktime(self.mocktime + TARGET_SPACING)
            pow_node.generatetoaddress(1, pow_node.getnewaddress())

        self.sync_hybrid_chain()
        assert_equal(pow_node.getblockcount(), FIRST_POS_HEIGHT)

        activation_block = self.nodes[OBSERVER_INDEX].getblock(
            self.nodes[OBSERVER_INDEX].getblockhash(FIRST_POS_HEIGHT), 2
        )
        assert_equal(is_pos_block(activation_block), False)
        assert_raises_rpc_error(
            -4,
            "No mature wallet outputs are available for staking",
            self.nodes[OBSERVER_INDEX].generatepos,
            1,
            1,
        )

        for node_index in staking_nodes:
            assert_equal(
                self.nodes[node_index].getstakingstatus()["mintablecoins"],
                True,
            )

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

        if node_index == 0:
            for peer_index in range(1, self.num_nodes):
                connect_nodes_bi(self.nodes, 0, peer_index)
        else:
            connect_nodes_bi(self.nodes, 0, node_index)
        self.sync_hybrid_chain()

    def identify_pos_producer(self, controller, block, staking_nodes):
        owners = [
            node_index
            for node_index in staking_nodes
            if controller.wallet_owns_coinstake(node_index, block)
        ]
        if len(owners) != 1:
            raise AssertionError(
                "Expected one staking wallet to own coinstake {}, found {}".format(
                    block["tx"][1]["txid"], owners
                )
            )
        return owners[0]

    def record_live_blocks(self, controller, start_height, pow_producers, staking_nodes):
        observer = self.nodes[OBSERVER_INDEX]
        end_height = observer.getblockcount()
        for height in range(start_height + 1, end_height + 1):
            block_hash = observer.getblockhash(height)
            block = observer.getblock(block_hash, 2)
            if is_pos_block(block):
                producer = self.identify_pos_producer(controller, block, staking_nodes)
            else:
                producer = pow_producers.get(block_hash)
            controller.inspect_block(
                block_hash,
                "live",
                None,
                producer,
            )
        return end_height

    def run_live(self, controller, pow_nodes, pos_nodes):
        if self.options.blocks < 1:
            raise ValueError("--blocks must be greater than zero")
        if self.options.pow_weight < 0 or self.options.pos_weight < 0:
            raise ValueError("PoW and PoS weights must not be negative")
        total_weight = self.options.pow_weight + self.options.pos_weight
        if total_weight == 0:
            raise ValueError("At least one production weight must be greater than zero")

        active_stakers = pos_nodes if self.options.pos_weight > 0 else []
        start_height = self.nodes[OBSERVER_INDEX].getblockcount()
        for node_index in active_stakers:
            self.restart_with_staking(node_index, True)

        randomizer = random.Random(self.options.seed)
        recorded_height = start_height
        deadline = time.time() + LIVE_WALL_TIMEOUT
        virtual_deadline = self.mocktime + LIVE_VIRTUAL_TIMEOUT
        pow_producers = {}

        self.sync_hybrid_chain()
        recorded_height = self.record_live_blocks(
            controller, recorded_height, pow_producers, active_stakers
        )

        try:
            while (
                len(controller.recorder.blocks) < self.options.blocks
                and time.time() < deadline
                and self.mocktime < virtual_deadline
            ):
                controller.advance_time()
                select_pow = randomizer.randrange(total_weight) < self.options.pow_weight
                if select_pow:
                    node_index = randomizer.choice(pow_nodes)
                    if node_index not in controller.pow_addresses:
                        controller.pow_addresses[node_index] = self.nodes[node_index].getnewaddress()
                    block_hash = self.nodes[node_index].generatetoaddress(
                        1, controller.pow_addresses[node_index]
                    )[0]
                    pow_producers[block_hash] = node_index
                else:
                    time.sleep(1)

                self.sync_hybrid_chain()
                recorded_height = self.record_live_blocks(
                    controller, recorded_height, pow_producers, active_stakers
                )
        finally:
            for node_index in active_stakers:
                self.restart_with_staking(node_index, False)

        self.sync_hybrid_chain()
        recorded_height = self.record_live_blocks(
            controller, recorded_height, pow_producers, active_stakers
        )
        if len(controller.recorder.blocks) < self.options.blocks:
            raise AssertionError(
                "Live mode produced {} of {} requested blocks. Replay with: {}".format(
                    len(controller.recorder.blocks),
                    self.options.blocks,
                    self.replay_command(),
                )
            )
        assert_equal(recorded_height, self.nodes[OBSERVER_INDEX].getblockcount())

    def run_test(self):
        pow_nodes = parse_node_indexes(self.options.pow_nodes, "--pow-nodes")
        pos_nodes = parse_node_indexes(self.options.pos_nodes, "--pos-nodes")
        self.validate_producers(pow_nodes, "--pow-nodes")
        self.validate_producers(pos_nodes, "--pos-nodes")
        if self.options.pos_attempts < 1:
            raise ValueError("--pos-attempts must be greater than zero")

        schedule_builder = HybridSchedule(
            self.options.mode,
            self.options.blocks,
            self.options.seed,
            self.options.pow_weight,
            self.options.pos_weight,
            pow_nodes,
            pos_nodes,
            self.options.sequence,
        )
        requested_order = schedule_builder.build()
        if self.options.mode == "scripted":
            scripted_nodes = [entry["node"] for entry in requested_order]
            self.validate_producers(scripted_nodes, "--sequence")

        if self.options.mode == "live":
            requested_pos_nodes = sorted(pos_nodes) if self.options.pos_weight > 0 else []
        else:
            requested_pos_nodes = sorted(
                set(
                    entry["node"]
                    for entry in requested_order
                    if entry["proof"] == PROOF_OF_STAKE
                )
            )
        self.bootstrap_chain(requested_pos_nodes)

        recorder = HybridRunRecorder(
            self.options.mode,
            self.options.seed,
            self.options.pow_weight,
            self.options.pos_weight,
            requested_order,
        )
        controller = HybridChainController(
            self.nodes,
            OBSERVER_INDEX,
            self.set_mocktime,
            self.sync_hybrid_chain,
            recorder,
            self.log,
            self.options.pos_attempts,
            TARGET_SPACING,
            self.replay_command(),
        )
        controller.set_time(self.mocktime)

        self.log.info(
            "Hybrid mode=%s seed=%s requested=%s",
            self.options.mode,
            self.options.seed,
            json.dumps(requested_order, sort_keys=True),
        )
        if self.options.mode == "live":
            self.run_live(controller, pow_nodes, pos_nodes)
        else:
            for request in requested_order:
                controller.produce(request)

        self.sync_hybrid_chain()
        best_hash = self.nodes[OBSERVER_INDEX].getbestblockhash()
        for node in self.nodes:
            assert_equal(node.getbestblockhash(), best_hash)
            assert_equal(node.verifychain(4, 0), True)

        record_path = self.options.record_file
        if not record_path:
            record_path = os.path.join(self.options.tmpdir, "hybrid_chain_record.json")
        recorder.write(record_path)
        self.log.info("Hybrid replay command: %s", self.replay_command())
        self.log.info("Hybrid run record: %s", json.dumps(recorder.as_dict(), sort_keys=True))
        self.log.info("Hybrid JSON record written to %s", record_path)


if __name__ == "__main__":
    HybridChainTest().main()
