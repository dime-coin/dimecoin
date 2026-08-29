#!/usr/bin/env python3
# Copyright (c) 2026 The Dimecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Helpers for controlled and reproducible hybrid PoW/PoS regtest chains."""

import json
import random
from decimal import Decimal

from .authproxy import JSONRPCException


PROOF_OF_WORK = "pow"
PROOF_OF_STAKE = "pos"


def is_pos_block(block):
    """Classify a verbose block using the existing coinstake transaction shape."""
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


def parse_node_indexes(value, option_name):
    """Parse a comma-separated node list while preserving its requested order."""
    indexes = []
    for item in value.split(","):
        item = item.strip()
        if not item:
            raise ValueError("{} contains an empty node index".format(option_name))
        try:
            index = int(item)
        except ValueError:
            raise ValueError("{} contains a non-numeric node index: {}".format(option_name, item))
        if index < 0:
            raise ValueError("{} contains a negative node index: {}".format(option_name, index))
        if index not in indexes:
            indexes.append(index)
    if not indexes:
        raise ValueError("{} must name at least one node".format(option_name))
    return indexes


def parse_scripted_sequence(value):
    """Parse proof:node entries such as pow:0,pos:1 into a block schedule."""
    schedule = []
    if not value.strip():
        raise ValueError("--sequence is required when --mode=scripted")

    for position, item in enumerate(value.split(","), 1):
        parts = item.strip().lower().split(":")
        if len(parts) != 2 or parts[0] not in (PROOF_OF_WORK, PROOF_OF_STAKE):
            raise ValueError(
                "Invalid sequence entry {}: expected pow:N or pos:N".format(position)
            )
        try:
            node_index = int(parts[1])
        except ValueError:
            raise ValueError("Invalid node index in sequence entry {}".format(position))
        if node_index < 0:
            raise ValueError("Negative node index in sequence entry {}".format(position))
        schedule.append({"proof": parts[0], "node": node_index})
    return schedule


class HybridSchedule:
    """Create scripted or seeded-random block requests without touching nodes."""

    def __init__(self, mode, blocks, seed, pow_weight, pos_weight, pow_nodes, pos_nodes, sequence=""):
        self.mode = mode
        self.blocks = blocks
        self.seed = seed
        self.pow_weight = pow_weight
        self.pos_weight = pos_weight
        self.pow_nodes = list(pow_nodes)
        self.pos_nodes = list(pos_nodes)
        self.sequence = sequence

    def build(self):
        """Return the complete requested order for controlled modes."""
        if self.mode == "scripted":
            return parse_scripted_sequence(self.sequence)
        if self.mode == "live":
            return []
        if self.mode != "random":
            raise ValueError("Unknown hybrid production mode: {}".format(self.mode))
        if self.blocks < 1:
            raise ValueError("--blocks must be greater than zero")
        if self.pow_weight < 0 or self.pos_weight < 0:
            raise ValueError("PoW and PoS weights must not be negative")
        if self.pow_weight + self.pos_weight == 0:
            raise ValueError("At least one of --pow-weight or --pos-weight must be greater than zero")
        if self.pow_weight > 0 and not self.pow_nodes:
            raise ValueError("A positive PoW weight requires at least one PoW producer")
        if self.pos_weight > 0 and not self.pos_nodes:
            raise ValueError("A positive PoS weight requires at least one PoS producer")

        randomizer = random.Random(self.seed)
        total_weight = self.pow_weight + self.pos_weight
        schedule = []
        for unused in range(self.blocks):
            if randomizer.randrange(total_weight) < self.pow_weight:
                proof = PROOF_OF_WORK
                producers = self.pow_nodes
            else:
                proof = PROOF_OF_STAKE
                producers = self.pos_nodes
            schedule.append({"proof": proof, "node": randomizer.choice(producers)})
        return schedule


class HybridRunRecorder:
    """Collect requested order, actual blocks, and unsuccessful production attempts."""

    def __init__(self, mode, seed, pow_weight, pos_weight, requested_order):
        self.mode = mode
        self.seed = seed
        self.pow_weight = pow_weight
        self.pos_weight = pos_weight
        self.requested_order = list(requested_order)
        self.blocks = []
        self.failed_attempts = []

    def add_failure(self, requested_proof, requested_node, attempt, timestamp, message):
        """Record one failed request without advancing the requested schedule."""
        failure = {
            "requested_proof": requested_proof,
            "requested_node": requested_node,
            "attempt": attempt,
            "timestamp": timestamp,
            "error": message,
        }
        self.failed_attempts.append(failure)
        return failure

    def add_block(self, requested_proof, requested_node, producing_node, block, failures):
        """Record a synchronized block after independently classifying its proof type."""
        actual_proof = PROOF_OF_STAKE if is_pos_block(block) else PROOF_OF_WORK
        record = {
            "requested_proof": requested_proof,
            "requested_node": requested_node,
            "producing_node": producing_node,
            "proof": actual_proof,
            "height": block["height"],
            "timestamp": block["time"],
            "hash": block["hash"],
            "chainwork": block["chainwork"],
            "failed_attempts_before_success": list(failures),
        }
        self.blocks.append(record)
        return record

    def as_dict(self):
        """Return the complete JSON-serializable run record."""
        return {
            "mode": self.mode,
            "seed": self.seed,
            "pow_weight": self.pow_weight,
            "pos_weight": self.pos_weight,
            "requested_order": self.requested_order,
            "actual_order": [
                {"proof": block["proof"], "node": block["producing_node"]}
                for block in self.blocks
            ],
            "blocks": self.blocks,
            "failed_production_attempts": self.failed_attempts,
        }

    def write(self, path):
        """Write the run record in a stable, human-readable JSON form."""
        with open(path, "w", encoding="utf8") as record_file:
            json.dump(self.as_dict(), record_file, indent=2, sort_keys=True)
            record_file.write("\n")


class HybridChainController:
    """Produce individual blocks through the existing PoW and PoS RPC paths."""

    def __init__(self, nodes, observer_index, set_mocktime, sync_blocks, recorder, log, pos_attempts, time_step, replay_command):
        self.nodes = nodes
        self.observer_index = observer_index
        self.set_mocktime = set_mocktime
        self.sync_blocks = sync_blocks
        self.recorder = recorder
        self.log = log
        self.pos_attempts = pos_attempts
        self.time_step = time_step
        self.replay_command = replay_command
        self.mocktime = None
        self.pow_addresses = {}

    def set_time(self, timestamp):
        """Initialize or replace the controller's shared node time."""
        self.mocktime = timestamp
        self.set_mocktime(timestamp)

    def advance_time(self):
        """Advance all nodes by one controlled production interval."""
        if self.mocktime is None:
            raise AssertionError("Hybrid controller time was not initialized")
        self.set_time(self.mocktime + self.time_step)
        return self.mocktime

    def inspect_block(self, block_hash, requested_proof, requested_node, producing_node, failures=None):
        """Synchronize and record one block from verbose data on the observer."""
        self.sync_blocks()
        block = self.nodes[self.observer_index].getblock(block_hash, 2)
        if "chainwork" not in block:
            block["chainwork"] = self.nodes[self.observer_index].getblockheader(block_hash)["chainwork"]
        record = self.recorder.add_block(
            requested_proof,
            requested_node,
            producing_node,
            block,
            failures or [],
        )
        if requested_proof in (PROOF_OF_WORK, PROOF_OF_STAKE) and record["proof"] != requested_proof:
            raise AssertionError(
                "Requested {} from node {} but block {} is {}".format(
                    requested_proof, requested_node, block_hash, record["proof"]
                )
            )
        if requested_proof == PROOF_OF_STAKE and not self.wallet_owns_coinstake(producing_node, block):
            raise AssertionError(
                "Requested PoS from node {} but its wallet does not contain the coinstake for block {}".format(
                    producing_node, block_hash
                )
            )
        return record

    def produce_pow(self, node_index):
        """Request one ordinary PoW block through unchanged generatetoaddress."""
        self.advance_time()
        if node_index not in self.pow_addresses:
            self.pow_addresses[node_index] = self.nodes[node_index].getnewaddress()
        block_hash = self.nodes[node_index].generatetoaddress(
            1, self.pow_addresses[node_index]
        )[0]
        return self.inspect_block(
            block_hash, PROOF_OF_WORK, node_index, node_index
        )

    def produce_pos(self, node_index):
        """Request one PoS block, advancing mock time after each failed kernel search."""
        failures = []
        for attempt in range(1, self.pos_attempts + 1):
            self.advance_time()
            try:
                hashes = self.nodes[node_index].generatepos(1, 1)
            except JSONRPCException as error:
                message = error.error["message"]
                if error.error["code"] != -1 or "Unable to find a valid proof-of-stake block" not in message:
                    raise
                failure = self.recorder.add_failure(
                    PROOF_OF_STAKE, node_index, attempt, self.mocktime, message
                )
                failures.append(failure)
                continue

            if len(hashes) != 1:
                raise AssertionError("generatepos returned {} hashes, expected one".format(len(hashes)))
            return self.inspect_block(
                hashes[0], PROOF_OF_STAKE, node_index, node_index, failures
            )

        raise AssertionError(
            "Node {} could not produce the requested PoS block after {} attempts. Replay with: {}".format(
                node_index, self.pos_attempts, self.replay_command
            )
        )

    def produce(self, request):
        """Dispatch one controlled schedule entry to its requested production path."""
        if request["proof"] == PROOF_OF_WORK:
            return self.produce_pow(request["node"])
        if request["proof"] == PROOF_OF_STAKE:
            return self.produce_pos(request["node"])
        raise ValueError("Unknown requested proof type: {}".format(request["proof"]))

    def wallet_owns_coinstake(self, node_index, block):
        """Return whether a node wallet contains a block's coinstake transaction."""
        if not is_pos_block(block):
            return False
        try:
            self.nodes[node_index].gettransaction(block["tx"][1]["txid"])
            return True
        except JSONRPCException as error:
            if error.error["code"] != -5:
                raise
            return False
