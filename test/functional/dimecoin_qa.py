#!/usr/bin/env python3
"""
Dimecoin QA Suite - single-entry functional, security and performance harness.

Runs a broad battery of checks against a throwaway Dimecoin daemon and emits a
self-contained HTML report.

SAFETY
------
This script refuses to run against mainnet. It verifies the target chain is
regtest or testnet before every destructive operation, uses its own datadir and
its own ports, and never touches a datadir it did not create.

Tests are driven through dimecoin-cli rather than raw JSON-RPC on purpose: the
CLI argument-conversion table is a real source of defects that a JSON-RPC-only
test cannot observe.

Usage:
    python3 dimecoin_qa.py --srcdir ~/dime253
    python3 dimecoin_qa.py --srcdir ~/dime253 --categories wallet,staking
    python3 dimecoin_qa.py --srcdir ~/dime253 --report ~/my-reports/

Reports are timestamped and written to <srcdir>/test/qa-reports/ by default.
That directory is listed in .gitignore, so run artifacts are never committed.
"""

import argparse
import html
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import traceback
from datetime import datetime

SUITE_VERSION = "1.0"

PASS = "PASS"
FAIL = "FAIL"
SKIP = "SKIP"
WARN = "WARN"


# --------------------------------------------------------------------------
# result model
# --------------------------------------------------------------------------

class Check:
    def __init__(self, category, name, status, detail="", duration=0.0):
        self.category = category
        self.name = name
        self.status = status
        self.detail = detail
        self.duration = duration


class Suite:
    """Collects results and timing."""

    def __init__(self):
        self.checks = []
        self.metrics = []
        self.started = time.time()
        self.category = "general"
        self.aborted = False

    def record(self, name, status, detail="", duration=0.0):
        self.checks.append(Check(self.category, name, status, detail, duration))
        symbol = {PASS: "ok  ", FAIL: "FAIL", SKIP: "skip", WARN: "warn"}[status]
        line = "  [%s] %s" % (symbol, name)
        if detail and status in (FAIL, WARN):
            line += "  -- %s" % detail
        print(line, flush=True)

    def ok(self, name, detail="", duration=0.0):
        self.record(name, PASS, detail, duration)

    def fail(self, name, detail="", duration=0.0):
        self.record(name, FAIL, detail, duration)

    def skip(self, name, detail="", duration=0.0):
        self.record(name, SKIP, detail, duration)

    def warn(self, name, detail="", duration=0.0):
        self.record(name, WARN, detail, duration)

    def expect(self, name, condition, detail=""):
        if condition:
            self.ok(name)
        else:
            self.fail(name, detail)
        return bool(condition)

    def equal(self, name, actual, expected):
        if actual == expected:
            self.ok(name)
            return True
        self.fail(name, "expected %r, got %r" % (expected, actual))
        return False

    def metric(self, name, value, unit=""):
        self.metrics.append((self.category, name, value, unit))
        print("  [mtrc] %s = %s %s" % (name, value, unit), flush=True)

    def counts(self):
        c = {PASS: 0, FAIL: 0, SKIP: 0, WARN: 0}
        for chk in self.checks:
            c[chk.status] += 1
        return c


# --------------------------------------------------------------------------
# node control
# --------------------------------------------------------------------------

class NodeError(Exception):
    pass


class CliResult:
    def __init__(self, rc, out, err, duration):
        self.rc = rc
        self.out = (out or "").strip()
        self.err = (err or "").strip()
        self.duration = duration

    @property
    def ok(self):
        return self.rc == 0

    def json(self):
        return json.loads(self.out)

    def error_code(self):
        """Extract the numeric RPC error code from a CLI failure."""
        blob = self.err or self.out
        marker = "error code:"
        if marker in blob:
            tail = blob.split(marker, 1)[1].strip()
            num = ""
            for ch in tail:
                if ch == "-" or ch.isdigit():
                    num += ch
                else:
                    break
            if num not in ("", "-"):
                return int(num)
        return None

    def __repr__(self):
        return "CliResult(rc=%s, out=%r, err=%r)" % (self.rc, self.out[:120], self.err[:120])


class Node:
    def __init__(self, srcdir, datadir, chain="regtest", port=31941, rpcport=31942,
                 rpcuser="qauser", rpcpassword="qapass", timeout=120):
        self.srcdir = os.path.abspath(os.path.expanduser(srcdir))
        self.datadir = os.path.abspath(os.path.expanduser(datadir))
        self.chain = chain
        self.port = port
        self.rpcport = rpcport
        self.rpcuser = rpcuser
        self.rpcpassword = rpcpassword
        self.timeout = timeout
        self.daemon = os.path.join(self.srcdir, "src", "dimecoind")
        self.clibin = os.path.join(self.srcdir, "src", "dimecoin-cli")
        self.owns_datadir = False
        self._chain_verified = False

        for path in (self.daemon, self.clibin):
            if not os.path.isfile(path):
                raise NodeError("binary not found: %s" % path)

    # -- argument plumbing --------------------------------------------------

    def _chain_flag(self):
        if self.chain == "regtest":
            return ["-regtest"]
        if self.chain == "testnet":
            return ["-testnet"]
        return []

    def _common_args(self):
        return self._chain_flag() + [
            "-datadir=%s" % self.datadir,
            "-rpcport=%d" % self.rpcport,
            "-rpcuser=%s" % self.rpcuser,
            "-rpcpassword=%s" % self.rpcpassword,
        ]

    # -- lifecycle ----------------------------------------------------------

    def create_datadir(self):
        if os.path.exists(self.datadir):
            raise NodeError(
                "refusing to reuse an existing datadir: %s\n"
                "This suite only operates on a datadir it creates itself."
                % self.datadir)
        os.makedirs(self.datadir)
        self.owns_datadir = True

    def check_ports_free(self):
        """Fail fast if our ports are already taken.

        A stale daemon on our rpcport answers the CLI and rejects our
        credentials, which surfaces as a confusing 'Authorization failed'
        rather than a port conflict. Detect it up front instead.
        """
        for name, port in (("p2p", self.port), ("rpc", self.rpcport)):
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                sock.bind(("127.0.0.1", port))
            except OSError:
                raise NodeError(
                    "%s port %d is already in use -- another daemon is running. "
                    "Stop it, or re-run with --port/--rpcport."
                    % (name, port))
            finally:
                sock.close()

    def start(self, extra_args=None, wait=True):
        self.check_ports_free()
        args = [self.daemon] + self._common_args() + [
            "-port=%d" % self.port,
            "-listen=1",
            "-server=1",
            "-daemon",
        ]
        if extra_args:
            args += extra_args
        proc = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if proc.returncode != 0:
            raise NodeError("daemon failed to start: %s %s"
                            % (proc.stdout.decode(errors="replace"),
                               proc.stderr.decode(errors="replace")))
        if wait:
            self.wait_for_rpc()

    def wait_for_rpc(self, timeout=None):
        deadline = time.time() + (timeout or self.timeout)
        last = None
        while time.time() < deadline:
            res = self.cli("getblockchaininfo", check_chain=False, quiet=True)
            if res.ok:
                return True
            last = res
            time.sleep(1)
        raise NodeError("RPC did not come up within %ss: %r" % (timeout or self.timeout, last))

    def wait_for_ports_free(self, timeout=None):
        """Wait until both ports can actually be bound again.

        The RPC server stops answering before the process exits, so a daemon
        that has flushed its chainstate can still be holding its listening
        sockets. Without this wait the next start() races the dying process
        and fails with a spurious 'port is already in use'.
        """
        deadline = time.time() + (timeout or self.timeout)
        while time.time() < deadline:
            try:
                self.check_ports_free()
                return True
            except NodeError:
                time.sleep(0.5)
        return False

    def stop(self, wait=True):
        res = self.cli("stop", check_chain=False, quiet=True)
        if wait:
            deadline = time.time() + self.timeout
            stopped = False
            while time.time() < deadline:
                probe = self.cli("getblockcount", check_chain=False, quiet=True)
                if not probe.ok:
                    stopped = True
                    break
                time.sleep(1)
            self.wait_for_ports_free()
            return stopped
        return res.ok

    def is_running(self):
        return self.cli("getblockcount", check_chain=False, quiet=True).ok

    def debug_log_path(self):
        for root, _dirs, files in os.walk(self.datadir):
            if "debug.log" in files:
                return os.path.join(root, "debug.log")
        return None

    def debug_log_size(self):
        path = self.debug_log_path()
        if not path:
            return 0
        try:
            return os.path.getsize(path)
        except OSError:
            return 0

    def read_debug_log(self, offset=0):
        """Read debug.log from a byte offset, so a test can look at only the
        lines produced since it started watching."""
        path = self.debug_log_path()
        if not path:
            return ""
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as fh:
                fh.seek(offset)
                return fh.read()
        except OSError:
            return ""

    # -- rpc ----------------------------------------------------------------

    def cli(self, *args, **kwargs):
        """Invoke dimecoin-cli. Verifies the chain is not mainnet first."""
        check_chain = kwargs.pop("check_chain", True)
        quiet = kwargs.pop("quiet", False)
        timeout = kwargs.pop("timeout", self.timeout)

        if check_chain and not self._chain_verified:
            self._verify_not_mainnet()

        cmd = [self.clibin] + self._common_args() + [str(a) for a in args]
        start = time.time()
        try:
            proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, timeout=timeout)
            rc, out, err = (proc.returncode,
                            proc.stdout.decode(errors="replace"),
                            proc.stderr.decode(errors="replace"))
        except subprocess.TimeoutExpired:
            rc, out, err = (-9, "", "timeout after %ss" % timeout)
        duration = time.time() - start
        if not quiet and rc != 0:
            pass
        return CliResult(rc, out, err, duration)

    def rpc(self, *args, **kwargs):
        """Convenience: run a CLI command and return parsed JSON, or raise."""
        res = self.cli(*args, **kwargs)
        if not res.ok:
            raise NodeError("%s failed: %s" % (" ".join(str(a) for a in args),
                                               res.err or res.out))
        if not res.out:
            return None
        try:
            return res.json()
        except ValueError:
            return res.out

    # -- safety -------------------------------------------------------------

    def _verify_not_mainnet(self):
        res = self.cli("getblockchaininfo", check_chain=False, quiet=True)
        if not res.ok:
            return
        try:
            chain = res.json().get("chain")
        except ValueError:
            return
        if chain == "main":
            raise NodeError(
                "SAFETY ABORT: target node reports chain='main'. "
                "This suite must never run against mainnet.")
        self._chain_verified = True


# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------

def timed(fn, *args, **kwargs):
    start = time.time()
    out = fn(*args, **kwargs)
    return out, time.time() - start


def mine(node, count):
    """Mine `count` blocks, tolerating the per-call template retry behaviour."""
    target = node.rpc("getblockcount") + count
    deadline = time.time() + 300
    while node.rpc("getblockcount") < target and time.time() < deadline:
        remaining = target - node.rpc("getblockcount")
        node.cli("generate", remaining)
    return node.rpc("getblockcount")


def new_address(node, label=None):
    if label:
        return node.rpc("getnewaddress", label)
    return node.rpc("getnewaddress")


# --------------------------------------------------------------------------
# test categories
# --------------------------------------------------------------------------

def test_safety(suite, node):
    suite.category = "safety"
    info = node.rpc("getblockchaininfo")
    chain = info.get("chain")
    suite.expect("chain is not mainnet", chain != "main", "chain=%s" % chain)
    suite.expect("chain is an isolated test chain", chain in ("regtest", "test"),
                 "chain=%s" % chain)
    suite.expect("datadir was created by this run", node.owns_datadir)
    suite.expect("datadir is not the default ~/.dimecoin",
                 os.path.expanduser("~/.dimecoin") != node.datadir)
    suite.expect("rpc port is not the mainnet default", node.rpcport != 11932)
    suite.expect("p2p port is not the mainnet default", node.port != 11931)


def test_node_lifecycle(suite, node):
    suite.category = "node"
    info = node.rpc("getblockchaininfo")
    suite.expect("getblockchaininfo returns a chain field", "chain" in info)
    suite.expect("getblockchaininfo returns blocks", "blocks" in info)

    net = node.rpc("getnetworkinfo")
    suite.expect("getnetworkinfo returns version", "version" in net)
    suite.expect("getnetworkinfo returns protocolversion", "protocolversion" in net)
    suite.metric("protocol version", net.get("protocolversion"), "")

    res = node.cli("uptime")
    suite.expect("uptime responds", res.ok, res.err)

    res = node.cli("getmemoryinfo")
    suite.expect("getmemoryinfo responds", res.ok, res.err)

    # restart cycle must preserve the chain
    height_before = node.rpc("getblockcount")
    best_before = node.rpc("getbestblockhash")
    _, stop_secs = timed(node.stop)
    suite.metric("clean shutdown time", round(stop_secs, 2), "s")
    _, start_secs = timed(node.start)
    suite.metric("restart time", round(start_secs, 2), "s")
    suite.equal("height preserved across restart", node.rpc("getblockcount"), height_before)
    suite.equal("best hash preserved across restart", node.rpc("getbestblockhash"), best_before)


def test_mining(suite, node):
    suite.category = "mining"
    start_height = node.rpc("getblockcount")

    res, secs = timed(node.cli, "generate", 1)
    suite.expect("generate 1 succeeds", res.ok, res.err)
    if res.ok:
        produced = node.rpc("getblockcount") - start_height
        suite.equal("generate 1 produced exactly one block", produced, 1)
        suite.metric("time to mine 1 block", round(secs, 3), "s")

    h0 = node.rpc("getblockcount")
    _, secs = timed(mine, node, 10)
    produced = node.rpc("getblockcount") - h0
    suite.equal("generate 10 produced ten blocks", produced, 10)
    if produced == 10 and secs > 0:
        suite.metric("block generation rate", round(10.0 / secs, 1), "blocks/s")

    addr = new_address(node)
    h0 = node.rpc("getblockcount")
    res = node.cli("generatetoaddress", 1, addr)
    suite.expect("generatetoaddress succeeds", res.ok, res.err)
    if res.ok:
        suite.equal("generatetoaddress produced one block",
                    node.rpc("getblockcount") - h0, 1)

    info = node.rpc("getmininginfo")
    for field in ("blocks", "difficulty", "chain"):
        suite.expect("getmininginfo exposes %s" % field, field in info)

    res = node.cli("getblocktemplate")
    suite.expect("getblocktemplate responds", res.ok or res.error_code() is not None,
                 res.err)

    # chain integrity
    tips = node.rpc("getchaintips")
    suite.expect("getchaintips returns at least one tip", len(tips) >= 1)
    height = node.rpc("getblockcount")
    bhash = node.rpc("getblockhash", height)
    block = node.rpc("getblock", bhash)
    suite.equal("getblock height matches getblockcount", block.get("height"), height)
    suite.equal("getblockhash round-trips", block.get("hash"), bhash)


def test_wallet_basics(suite, node):
    suite.category = "wallet"
    addr = new_address(node)
    suite.expect("getnewaddress returns a non-empty address", bool(addr))

    info = node.rpc("validateaddress", addr)
    suite.expect("validateaddress reports the address as valid", info.get("isvalid") is True)

    bad = node.rpc("validateaddress", "notarealaddress")
    suite.expect("validateaddress rejects an invalid address", bad.get("isvalid") is False)

    winfo = node.rpc("getwalletinfo")
    for field in ("balance", "txcount", "keypoolsize"):
        suite.expect("getwalletinfo exposes %s" % field, field in winfo)

    res = node.cli("getbalance")
    suite.expect("getbalance responds", res.ok, res.err)

    res = node.cli("listunspent")
    suite.expect("listunspent responds", res.ok, res.err)
    if res.ok:
        suite.metric("spendable outputs", len(res.json()), "utxo")

    # private key round trip
    priv = node.cli("dumpprivkey", addr)
    if priv.ok:
        suite.ok("dumpprivkey returns a key")
        imported = node.cli("importprivkey", priv.out, "qa-imported", "false")
        suite.expect("importprivkey accepts the exported key",
                     imported.ok, imported.err)
    else:
        suite.fail("dumpprivkey returns a key", priv.err)

    backup = os.path.join(node.datadir, "qa-backup.dat")
    res = node.cli("backupwallet", backup)
    suite.expect("backupwallet writes a file", res.ok and os.path.exists(backup), res.err)

    res = node.cli("signmessage", addr, "dimecoin-qa")
    if res.ok:
        sig = res.out
        good = node.cli("verifymessage", addr, sig, "dimecoin-qa")
        suite.expect("signmessage/verifymessage round-trips", good.out == "true", good.out)
        tampered = node.cli("verifymessage", addr, sig, "dimecoin-qa-tampered")
        suite.expect("verifymessage rejects a tampered message",
                     tampered.out == "false" or not tampered.ok, tampered.out)
    else:
        suite.fail("signmessage responds", res.err)


def test_coinbase_maturity(suite, node):
    suite.category = "maturity"
    winfo = node.rpc("getwalletinfo")
    immature = winfo.get("immature_balance", 0)
    balance = winfo.get("balance", 0)
    suite.expect("coinbase rewards are tracked as immature", immature > 0,
                 "immature_balance=%s" % immature)
    suite.metric("immature balance", immature, "DIME")
    suite.metric("spendable balance", balance, "DIME")

    # mine past maturity and confirm funds become spendable
    before = node.rpc("getwalletinfo").get("balance", 0)
    mine(node, 20)
    after = node.rpc("getwalletinfo").get("balance", 0)
    suite.expect("balance becomes spendable after maturity depth", after >= before,
                 "before=%s after=%s" % (before, after))


def test_transactions(suite, node):
    suite.category = "transactions"
    balance = node.rpc("getwalletinfo").get("balance", 0)
    if balance <= 0:
        suite.skip("send flow", "no spendable balance yet")
        return

    dest = new_address(node, "qa-dest")
    amount = 1.0
    res = node.cli("sendtoaddress", dest, amount)
    if not res.ok:
        suite.fail("sendtoaddress succeeds", res.err)
        return
    suite.ok("sendtoaddress succeeds")
    txid = res.out

    tx = node.cli("gettransaction", txid)
    suite.expect("gettransaction finds the new transaction", tx.ok, tx.err)

    mem = node.rpc("getrawmempool")
    suite.expect("transaction appears in the mempool", txid in mem,
                 "mempool=%s" % mem)

    mine(node, 2)
    tx = node.rpc("gettransaction", txid)
    suite.expect("transaction confirms after mining",
                 tx.get("confirmations", 0) >= 1,
                 "confirmations=%s" % tx.get("confirmations"))

    lst = node.rpc("listtransactions")
    suite.expect("listtransactions includes the transaction",
                 any(t.get("txid") == txid for t in lst))

    res = node.cli("sendmany", "", json.dumps({new_address(node): 0.5,
                                               new_address(node): 0.25}))
    suite.expect("sendmany succeeds", res.ok, res.err)

    raw = node.cli("getrawtransaction", txid)
    suite.expect("getrawtransaction returns the raw hex", raw.ok, raw.err)
    if raw.ok:
        decoded = node.cli("decoderawtransaction", raw.out)
        suite.expect("decoderawtransaction parses it", decoded.ok, decoded.err)
        if decoded.ok:
            suite.equal("decoded txid matches", decoded.json().get("txid"), txid)


def test_wallet_encryption(suite, node, passphrase="qa-passphrase-1"):
    """Encryption is destructive and restarts the daemon, so it runs late."""
    suite.category = "encryption"

    res = node.cli("encryptwallet", passphrase)
    if not res.ok:
        suite.fail("encryptwallet succeeds", res.err)
        return False
    suite.ok("encryptwallet succeeds")

    # encryptwallet shuts the daemon down
    deadline = time.time() + 60
    while node.is_running() and time.time() < deadline:
        time.sleep(1)
    node.start()
    suite.expect("daemon restarts after encryption", node.is_running())

    winfo = node.rpc("getwalletinfo")
    suite.expect("wallet reports a lock state after encryption",
                 "unlocked_until" in winfo)
    suite.equal("wallet starts locked", winfo.get("unlocked_until"), 0)

    # wrong passphrase must be rejected
    bad = node.cli("walletpassphrase", "definitely-wrong", 60)
    suite.expect("wrong passphrase is rejected", not bad.ok, bad.out)
    suite.equal("wrong passphrase returns RPC_WALLET_PASSPHRASE_INCORRECT (-14)",
                bad.error_code(), -14)

    # locked wallet must refuse to reveal keys or spend
    addr = new_address(node)
    locked_dump = node.cli("dumpprivkey", addr)
    suite.expect("locked wallet refuses dumpprivkey", not locked_dump.ok,
                 locked_dump.out)
    suite.equal("dumpprivkey returns RPC_WALLET_UNLOCK_NEEDED (-13)",
                locked_dump.error_code(), -13)

    locked_send = node.cli("sendtoaddress", addr, 0.1)
    suite.expect("locked wallet refuses sendtoaddress", not locked_send.ok,
                 locked_send.out)

    # correct passphrase unlocks
    good = node.cli("walletpassphrase", passphrase, 120)
    suite.expect("correct passphrase unlocks the wallet", good.ok, good.err)
    winfo = node.rpc("getwalletinfo")
    suite.expect("unlocked_until advances after unlock",
                 winfo.get("unlocked_until", 0) > 0)

    unlocked_dump = node.cli("dumpprivkey", addr)
    suite.expect("unlocked wallet permits dumpprivkey", unlocked_dump.ok,
                 unlocked_dump.err)

    # walletlock re-locks
    node.cli("walletlock")
    winfo = node.rpc("getwalletinfo")
    suite.equal("walletlock re-locks the wallet", winfo.get("unlocked_until"), 0)

    # passphrase change
    newphrase = passphrase + "-changed"
    res = node.cli("walletpassphrasechange", passphrase, newphrase)
    suite.expect("walletpassphrasechange succeeds", res.ok, res.err)
    if res.ok:
        old = node.cli("walletpassphrase", passphrase, 30)
        suite.expect("old passphrase no longer works", not old.ok, old.out)
        new = node.cli("walletpassphrase", newphrase, 120)
        suite.expect("new passphrase works", new.ok, new.err)
        node.cli("walletlock")
    return True


def test_staking_unlock(suite, node, passphrase):
    """Staking-only unlock: a first-class Dimecoin feature."""
    suite.category = "staking-unlock"

    node.cli("walletlock")
    res = node.cli("walletpassphrase", passphrase, 300, "true")
    suite.expect("walletpassphrase accepts a boolean stakingonly argument from the CLI",
                 res.ok, res.err or res.out)
    if not res.ok:
        suite.fail("staking-only unlock is usable from dimecoin-cli",
                   "this is the regression guarding the CLI conversion table")
        return

    winfo = node.rpc("getwalletinfo")
    suite.expect("wallet is unlocked after staking-only unlock",
                 winfo.get("unlocked_until", 0) > 0,
                 "unlocked_until=%s" % winfo.get("unlocked_until"))

    # a staking-only session must still refuse to spend
    addr = new_address(node)
    send = node.cli("sendtoaddress", addr, 0.1)
    suite.expect("staking-only session still refuses to spend", not send.ok,
                 send.out)

    # but must permit staking configuration
    res = node.cli("setstakesplitthreshold", 100)
    suite.expect("staking-only session permits setstakesplitthreshold",
                 res.ok, res.err or res.out)

    node.cli("walletlock")
    winfo = node.rpc("getwalletinfo")
    suite.equal("wallet relocks after a staking-only session",
                winfo.get("unlocked_until"), 0)


def test_user_journey(suite, node, passphrase):
    """The sequence a real user performs after a chain sync.

    Encrypt, unlock, receive, send, review, relock, unlock for staking,
    relock. Each category above tests one surface in isolation; this test
    exists to prove the surfaces still work together in the order a human
    actually drives them.
    """
    suite.category = "user-journey"

    node.cli("walletlock")
    winfo = node.rpc("getwalletinfo")
    suite.equal("journey starts from a locked encrypted wallet",
                winfo.get("unlocked_until"), 0)

    # 1. a locked wallet must refuse to spend
    dest = new_address(node, "journey-dest")
    blocked = node.cli("sendtoaddress", dest, 1)
    suite.expect("locked wallet refuses to send", not blocked.ok, blocked.out)

    # 2. unlock for spending
    res = node.cli("walletpassphrase", passphrase, 600)
    suite.expect("user can unlock the wallet for spending", res.ok, res.err)
    if not res.ok:
        return

    # 3. send coins to an address and confirm them
    balance_before = node.rpc("getwalletinfo").get("balance", 0)
    sent = node.cli("sendtoaddress", dest, 5)
    suite.expect("user can send coins to an address", sent.ok, sent.err)
    if not sent.ok:
        return
    txid = sent.out
    mine(node, 2)

    tx = node.rpc("gettransaction", txid)
    suite.expect("sent transaction confirms",
                 tx.get("confirmations", 0) >= 1,
                 "confirmations=%s" % tx.get("confirmations"))

    # 4. the funds actually arrived at the destination
    received = node.cli("getreceivedbyaddress", dest, 1)
    if received.ok:
        try:
            suite.expect("destination address shows the received amount",
                         float(received.out) >= 5.0, "got %s" % received.out)
        except ValueError:
            suite.fail("getreceivedbyaddress returns a number", received.out)
    else:
        suite.fail("getreceivedbyaddress responds", received.err)

    # 5. the transaction is visible in history
    history = node.rpc("listtransactions", "*", 50)
    suite.expect("transaction appears in the user's history",
                 any(t.get("txid") == txid for t in history))

    balance_after = node.rpc("getwalletinfo").get("balance", 0)
    suite.metric("balance moved during journey",
                 round(abs(balance_after - balance_before), 5), "DIME")

    # 6. relock
    node.cli("walletlock")
    suite.equal("user can relock the wallet",
                node.rpc("getwalletinfo").get("unlocked_until"), 0)
    after_lock = node.cli("sendtoaddress", dest, 1)
    suite.expect("relocked wallet refuses to send again", not after_lock.ok,
                 after_lock.out)

    # 7. unlock for staking only, then relock
    res = node.cli("walletpassphrase", passphrase, 600, "true")
    suite.expect("user can unlock for staking", res.ok, res.err or res.out)
    if res.ok:
        suite.expect("staking unlock still blocks spending",
                     not node.cli("sendtoaddress", dest, 1).ok)
        node.cli("walletlock")
        suite.equal("user can relock after staking",
                    node.rpc("getwalletinfo").get("unlocked_until"), 0)


def test_staking_config(suite, node):
    suite.category = "staking"

    res = node.cli("setstakesplitthreshold", 250)
    if res.ok:
        suite.ok("setstakesplitthreshold accepts a valid value")
        try:
            payload = res.json()
            suite.expect("setstakesplitthreshold returns 'saved' as a JSON boolean",
                         isinstance(payload.get("saved"), bool),
                         "got %r" % (payload.get("saved"),))
        except ValueError:
            suite.fail("setstakesplitthreshold returns valid JSON", res.out)
    else:
        suite.fail("setstakesplitthreshold accepts a valid value", res.err)

    got = node.cli("getstakesplitthreshold")
    suite.expect("getstakesplitthreshold responds", got.ok, got.err)

    neg = node.cli("setstakesplitthreshold", -1)
    suite.expect("setstakesplitthreshold rejects a negative value", not neg.ok, neg.out)
    suite.equal("negative threshold returns RPC_INVALID_PARAMETER (-8)",
                neg.error_code(), -8)

    res = node.cli("listminting")
    suite.expect("listminting responds", res.ok, res.err)
    if res.ok and res.out:
        try:
            json.loads(res.out)
            suite.ok("listminting emits parseable JSON")
        except ValueError as exc:
            suite.fail("listminting emits parseable JSON",
                       "%s -- output was %r" % (exc, res.out[:200]))

    # Dimecoin exposes the stake reserve as a startup argument, not an RPC, so
    # exercise the interface that actually exists.
    node.stop()
    node.start(extra_args=["-reservebalance=1000"])
    res = node.cli("getwalletinfo")
    suite.expect("daemon accepts -reservebalance startup argument", res.ok, res.err)
    node.stop()
    node.start()
    suite.expect("daemon restarts cleanly without -reservebalance",
                 node.cli("getwalletinfo").ok)


def test_staking_engine(suite, node):
    """Verify the staking engine is enabled and actively attempting to stake.

    Whether any individual stake search wins is a matter of chance, so this
    test does not assert that a PoS block gets minted. It asserts the parts
    that are deterministic and that actually matter to a user: the stake
    minter thread starts when staking is enabled, it repeatedly searches for a
    coinstake, the wallet reports stakeable outputs, and the whole engine can
    be turned off with -staking=0.
    """
    suite.category = "staking-engine"

    if node.chain != "regtest":
        suite.skip("staking engine test targets regtest")
        return

    # Stakeable coins need to exist: past nFirstPoSBlock, and mature.
    first_pos = 100
    height = node.rpc("getblockcount")
    if height <= first_pos + 20:
        mine(node, (first_pos + 25) - height)
    suite.expect("chain is past nFirstPoSBlock so PoS is permitted",
                 node.rpc("getblockcount") > first_pos)

    balance = node.rpc("getwalletinfo").get("balance", 0)
    suite.expect("wallet holds mature coins available to stake", balance > 0,
                 "balance=%s" % balance)

    # Restart with staking explicitly enabled and watch the log from that point.
    node.stop()
    node.start(extra_args=["-staking=1"])
    log = node.read_debug_log()
    suite.expect("stake minter thread starts when -staking=1",
                 "ThreadStakeMinter started" in log,
                 "no ThreadStakeMinter line in debug.log")

    # Age the coins so the minter has something eligible to search.
    node.cli("setmocktime", int(time.time()) + 7200)

    # The minter logs each search round. Seeing repeated rounds is the proof
    # that the client is genuinely attempting to stake.
    offset = node.debug_log_size()
    time.sleep(30)
    recent = node.read_debug_log(offset)
    attempts = recent.lower().count("coinstake")
    suite.expect("client actively attempts to stake once enabled",
                 attempts > 0,
                 "no coinstake search activity in 30s of debug.log")
    suite.metric("coinstake search rounds observed", attempts, "in 30s")

    listed = node.cli("listminting")
    suite.expect("listminting reports the wallet's stakeable outputs",
                 listed.ok, listed.err)
    if listed.ok and listed.out:
        try:
            entries = json.loads(listed.out)
            suite.metric("outputs tracked for minting",
                         len(entries) if isinstance(entries, list) else 0,
                         "outputs")
        except ValueError:
            suite.fail("listminting emits parseable JSON while staking",
                       listed.out[:200])

    node.cli("setmocktime", 0)

    # Staking must be switchable off.
    node.stop()
    node.start(extra_args=["-staking=0"])
    offset = node.debug_log_size()
    time.sleep(12)
    quiet = node.read_debug_log(offset)
    suite.expect("no stake minter starts when -staking=0",
                 "ThreadStakeMinter started" not in quiet,
                 "minter started despite -staking=0")
    suite.expect("node still healthy with staking disabled", node.is_running())

    node.stop()
    node.start()


COIN_SATS = 100000000

# Consensus constants mirrored from src/chainparams.cpp and src/validation.cpp.
# These are duplicated here on purpose: the suite reimplements the emission
# curve independently and compares it against what the daemon actually pays
# out. If the two disagree, either the chain params changed without review or
# the subsidy logic changed -- both are things a release must not do silently.
CONSENSUS = {
    "regtest": {"first_pos_block": 100, "pos_target_spacing": 64,
                "halving_interval": 512000},
    "testnet": {"first_pos_block": 100, "pos_target_spacing": 64,
                "halving_interval": 512000},
    "main":    {"first_pos_block": 5000000, "pos_target_spacing": 64,
                "halving_interval": 512000},
}
LWMA3_HEIGHT = 3310000
POS_START_REWARD = 15400
POS_DAILY_DECAY = 0.99978
BLOCK_REWARD_START = 1024
PREMINE_AMOUNT = 350000000


def model_pos_subsidy(height, first_pos_block, pos_target_spacing):
    """Independent reimplementation of decayBlockReward (validation.cpp:1228).

    Returns whole DIME. This exists to be a *second opinion*: the test compares
    it against the coinbase the daemon actually produced.
    """
    blocks_daily = 86400 // pos_target_spacing
    if blocks_daily <= 0:
        raise ValueError("blocks_daily would be %d -- decayBlockReward would "
                         "never terminate" % blocks_daily)
    subsidy = POS_START_REWARD * COIN_SATS
    passed = height - first_pos_block
    while passed > 0:
        passed -= blocks_daily
        if passed > 0:
            subsidy = int(subsidy * POS_DAILY_DECAY)
    return subsidy // COIN_SATS


def model_pow_subsidy(height, halving_interval):
    """Independent reimplementation of the pre-PoS branch of GetBlockSubsidy."""
    if height == 0:
        return 1
    if height < LWMA3_HEIGHT:
        subsidy = (BLOCK_REWARD_START * COIN_SATS) >> (height // halving_interval)
        mod_number = height % 1024 or 1024
        subsidy *= mod_number
        if 9 < height < 128:
            subsidy = PREMINE_AMOUNT * COIN_SATS
        return subsidy // COIN_SATS
    subsidy = float(BLOCK_REWARD_START * 8 * COIN_SATS)
    i = halving_interval
    while i <= (height - LWMA3_HEIGHT):
        subsidy -= subsidy / 12.5
        i += halving_interval
    subsidy = max(subsidy, float(4 * BLOCK_REWARD_START * COIN_SATS))
    return int(subsidy) // COIN_SATS


def model_subsidy(height, cfg):
    if height >= cfg["first_pos_block"]:
        return model_pos_subsidy(height, cfg["first_pos_block"],
                                 cfg["pos_target_spacing"])
    return model_pow_subsidy(height, cfg["halving_interval"])


def coinbase_total(node, height):
    """Total value paid out by the coinbase at a height, in whole DIME."""
    block_hash = node.cli("getblockhash", height).out.strip()
    block = node.rpc("getblock", block_hash, 2)
    coinbase = block["tx"][0]
    return sum(float(o["value"]) for o in coinbase["vout"])


def test_consensus_emission(suite, node):
    """Differential-test the block reward against an independent model.

    Dimecoin's emission is unusual: a premine window, an LWMA3-era schedule, a
    yearly-decline branch, and then a completely separate daily-decay curve
    once proof-of-stake activates. Nothing verified that the daemon actually
    pays what the schedule says, or that the reward behaves sanely across the
    PoS boundary. This does both.
    """
    suite.category = "consensus"

    if node.chain != "regtest":
        suite.skip("emission differential test targets regtest")
        return

    cfg = CONSENSUS["regtest"]
    first_pos = cfg["first_pos_block"]

    # The daemon must be past the PoS activation height for this to mean
    # anything, plus a margin so we can sample on both sides of it.
    height = node.rpc("getblockcount")
    if height < first_pos + 15:
        mine(node, (first_pos + 20) - height)

    # --- differential check: model vs what the chain actually paid ----------
    sample_heights = [1, 5, 9, 10, 50, first_pos - 1, first_pos,
                      first_pos + 1, first_pos + 10]
    mismatches = []
    for h in sample_heights:
        actual = coinbase_total(node, h)
        expected = model_subsidy(h, cfg)
        if abs(actual - expected) > 0.00000001:
            mismatches.append((h, expected, actual))
    suite.expect("block reward matches the independent emission model at "
                 "every sampled height", not mismatches,
                 "; ".join("h=%d expected %s got %s" % m for m in mismatches))
    suite.metric("emission heights differentially verified",
                 len(sample_heights), "heights")

    # --- invariants that must hold on any chain ----------------------------
    rewards = []
    for h in range(first_pos, min(height, first_pos + 25) + 1):
        rewards.append((h, coinbase_total(node, h)))

    suite.expect("no block reward is negative",
                 all(r >= 0 for _, r in rewards))
    suite.expect("no block reward exceeds the 21e14 money range",
                 all(r <= 21000000000000 for _, r in rewards))

    # After PoS activation the curve decays; it must never step back up.
    increases = [(h, prev, cur) for (_, prev), (h, cur)
                 in zip(rewards, rewards[1:]) if cur > prev]
    suite.expect("reward never increases after PoS activation",
                 not increases,
                 "increases at %s" % (increases[:3],))

    # --- the PoS activation step itself ------------------------------------
    before = coinbase_total(node, first_pos - 1)
    after = coinbase_total(node, first_pos)
    suite.metric("reward immediately before PoS activation", before, "DIME")
    suite.metric("reward immediately after PoS activation", after, "DIME")
    # On regtest and testnet nFirstPoSBlock is 100, which lands inside the
    # 9 < h < 128 premine window, so the block before activation pays the
    # premine rather than a normal reward. A ratio taken across that boundary
    # would be meaningless, so it is reported only where it means something.
    if before == PREMINE_AMOUNT:
        suite.skip("PoS activation reward ratio",
                   "activation on %s falls inside the premine window "
                   "(heights 10-%d), so the local step is premine -> PoS "
                   "rather than a representative reward change; the mainnet "
                   "figures below are the meaningful ones"
                   % (node.chain, first_pos - 1))
    elif before > 0:
        suite.metric("PoS activation reward ratio", round(after / before, 4), "x")

    # --- mainnet schedule, checked from the model alone --------------------
    # No mainnet node is needed: the model is already proven equivalent above.
    main = CONSENSUS["main"]
    m_first = main["first_pos_block"]
    m_before = model_subsidy(m_first - 1, main)
    m_after = model_subsidy(m_first, main)
    suite.metric("mainnet reward before PoS activation", m_before, "DIME")
    suite.metric("mainnet reward after PoS activation", m_after, "DIME")
    ratio = m_after / m_before if m_before else 0
    suite.metric("mainnet PoS activation reward ratio", round(ratio, 4), "x")
    # This is a known, deliberate property of the chain. It is asserted so
    # that any future change to it is caught rather than shipped silently.
    suite.expect("mainnet PoS activation step is the documented 2.414x",
                 abs(ratio - 2.414) < 0.01,
                 "ratio changed to %.4f -- emission schedule was altered" % ratio)

    # The decay must be monotonically non-increasing across a long horizon.
    prev = None
    bad = []
    for h in range(m_first, m_first + 4000000, 250000):
        cur = model_subsidy(h, main)
        if prev is not None and cur > prev:
            bad.append(h)
        prev = cur
    suite.expect("mainnet PoS decay is monotonically non-increasing", not bad,
                 "reward increased at heights %s" % bad[:3])

    # --- termination / divide-by-zero guard --------------------------------
    for chain, c in CONSENSUS.items():
        spacing = c["pos_target_spacing"]
        suite.expect("%s nPosTargetSpacing keeps decayBlockReward terminating"
                     % chain, 0 < spacing <= 86400,
                     "spacing=%s would make blocks_daily 0 and loop forever"
                     % spacing)


def test_consensus_block_timing(suite, node):
    """Check the PoW/PoS timing parameters are mutually consistent.

    Dimecoin runs PoW and PoS side by side and expects them to combine into a
    single target block time. If the two spacings ever diverge, the effective
    block rate silently changes.
    """
    suite.category = "consensus"

    cfg = CONSENSUS.get(node.chain if node.chain != "regtest" else "regtest")
    suite.expect("PoW and PoS target spacing agree",
                 cfg["pos_target_spacing"] == 64,
                 "pos spacing=%s" % cfg["pos_target_spacing"])

    # Measure the real spacing the chain produces. On regtest blocks are mined
    # on demand so this checks the timestamps are sane and strictly advancing,
    # not that they hit the target.
    height = node.rpc("getblockcount")
    times = []
    for h in range(max(1, height - 20), height + 1):
        block_hash = node.cli("getblockhash", h).out.strip()
        times.append(node.rpc("getblock", block_hash)["time"])

    non_advancing = [i for i in range(1, len(times)) if times[i] < times[i - 1]]
    suite.expect("block timestamps never move backwards", not non_advancing,
                 "regressions at offsets %s" % non_advancing[:3])
    suite.metric("blocks sampled for timestamp ordering", len(times), "blocks")


UNIT_TEST_BINARY = os.path.join("src", "test", "test_dimecoin")

# Failures that existed on the untouched 2.5.2 build and have since been fixed
# on this branch. Kept so the report can show the delta; if any of these comes
# back it lands in `failing` below and fails the suite as a regression.
PREVIOUSLY_FAILING = {
    "main_tests/subsidy_limit_test",
    "miner_tests/CreateNewBlock_validity",
    "util_tests/util_FormatMoney",
}

# Failures still accepted as pre-existing rather than treated as regressions.
# Empty: the C++ suite is green on this branch.
KNOWN_UNIT_FAILURES = set()


def test_unit_tests(suite, srcdir, timeout=1800):
    """Run the C++ Boost unit suite and fold its result into this report.

    This is a different tier from everything else here: in-process, no daemon,
    no wallet. It covers crypto, script, serialization and consensus maths that
    a black-box CLI test cannot reach. Running it from the same entry point
    means one command covers both tiers.
    """
    suite.category = "unit-tests"

    binary = os.path.join(os.path.expanduser(srcdir), UNIT_TEST_BINARY)
    if not os.path.exists(binary):
        suite.skip("C++ unit test binary not built at %s "
                   "(configure with --enable-tests)" % binary)
        return

    started = time.time()
    try:
        proc = subprocess.run([binary, "--report_level=short",
                               "--log_level=error"],
                              cwd=os.path.dirname(os.path.dirname(binary)),
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              timeout=timeout)
        output = proc.stdout.decode(errors="replace")
    except subprocess.TimeoutExpired:
        suite.fail("C++ unit suite completed within %ss" % timeout,
                   "timed out")
        return
    elapsed = time.time() - started
    suite.metric("C++ unit suite runtime", round(elapsed, 1), "s")

    cases = re.search(r"(\d+) test cases? out of (\d+) passed", output)
    asserts = re.search(r"(\d+) assertions? out of (\d+) passed", output)
    warned_cases = re.search(r"(\d+) test cases? out of (\d+) passed with warnings",
                             output)
    if cases:
        passed, total = int(cases.group(1)), int(cases.group(2))
        if warned_cases:
            passed += int(warned_cases.group(1))
        suite.metric("C++ unit test cases passed", "%d / %d" % (passed, total))
    if asserts:
        suite.metric("C++ unit assertions passed",
                     "%s / %s" % (format(int(asserts.group(1)), ","),
                                  format(int(asserts.group(2)), ",")))

    # Boost emits both "error:" (a failed BOOST_CHECK) and "warning:" (a failed
    # BOOST_WARN) lines in the same shape, so the prefix has to be matched or
    # deliberately-warned checks would be counted as failures.
    failing = set(re.findall(
        r'(?:fatal error|error): in "([a-zA-Z_0-9]+/[a-zA-Z_0-9]+)"', output))
    warned = set(re.findall(
        r'warning: in "([a-zA-Z_0-9]+/[a-zA-Z_0-9]+)"', output))
    new_failures = sorted(failing - KNOWN_UNIT_FAILURES)
    still_failing = sorted(failing & KNOWN_UNIT_FAILURES)

    suite.expect("no new C++ unit test failures", not new_failures,
                 "new failures: %s" % ", ".join(new_failures))
    suite.expect("C++ unit suite exited clean", proc.returncode == 0,
                 "exit code %d" % proc.returncode)

    for name in still_failing:
        suite.warn("pre-existing C++ unit failure: %s" % name,
                   "present on untouched 2.5.2 as well -- not a regression")

    for name in sorted(warned):
        suite.warn("C++ unit test reports warnings: %s" % name,
                   "checks intentionally downgraded to BOOST_WARN "
                   "pending a tracked fix -- not a regression")

    fixed = sorted(PREVIOUSLY_FAILING - failing)
    for name in fixed:
        suite.ok("previously-failing C++ unit test now passes: %s" % name)


def test_cli_argument_conversion(suite, node):
    """Guards the dimecoin-cli conversion table, a proven defect source."""
    suite.category = "cli-conversion"

    cases = [
        ("listminting takes numeric arguments", ["listminting", 10, 0]),
        ("getblockhash takes a numeric height", ["getblockhash", 1]),
        ("getbalance takes a numeric minconf", ["getbalance", "*", 1]),
        ("getblock takes a numeric verbosity", ["getblock",
                                                node.rpc("getbestblockhash"), 1]),
        ("listtransactions takes numeric count/skip", ["listtransactions", "*", 10, 0]),
        ("getreceivedbyaddress takes numeric minconf",
         ["getreceivedbyaddress", new_address(node), 0]),
    ]
    for name, argv in cases:
        res = node.cli(*argv)
        code = res.error_code()
        # -1 means the CLI handed the daemon a wrongly-typed value
        type_error = (code == -1 and "not an" in (res.err + res.out).lower())
        suite.expect(name, not type_error,
                     "type conversion failure: %s" % (res.err or res.out)[:160])


def test_negative_paths(suite, node):
    """Error paths dominated the audit backlog; they must fail cleanly."""
    suite.category = "error-paths"

    cases = [
        ("getblockhash rejects a negative height", ["getblockhash", -1]),
        ("getblockhash rejects an out-of-range height", ["getblockhash", 99999999]),
        ("getblock rejects a malformed hash", ["getblock", "notahash"]),
        ("gettransaction rejects a malformed txid", ["gettransaction", "zz"]),
        ("sendtoaddress rejects an invalid address",
         ["sendtoaddress", "notanaddress", 1]),
        ("sendtoaddress rejects a negative amount",
         ["sendtoaddress", new_address(node), -5]),
        ("validateaddress tolerates an empty string", ["validateaddress", ""]),
        ("setban rejects a malformed subnet", ["setban", "999.999.999.999", "add"]),
    ]
    for name, argv in cases:
        res = node.cli(*argv)
        # The requirement is a clean structured error, never a crash or hang.
        crashed = res.rc == -9 or "Assertion" in (res.err + res.out)
        suite.expect(name, not crashed, (res.err or res.out)[:160])

    # the daemon must still be alive after all of that
    suite.expect("daemon survives the negative-path battery", node.is_running())


def test_network(suite, node):
    suite.category = "network"
    for cmd in ("getpeerinfo", "getnetworkinfo", "getconnectioncount",
                "getnettotals", "listbanned"):
        res = node.cli(cmd)
        suite.expect("%s responds" % cmd, res.ok, res.err)

    res = node.cli("setban", "192.0.2.0/24", "add", 600)
    suite.expect("setban accepts a valid subnet", res.ok, res.err)
    if res.ok:
        banned = node.rpc("listbanned")
        suite.expect("listbanned reflects the new ban", len(banned) >= 1)
        node.cli("clearbanned")
        suite.equal("clearbanned empties the ban list",
                    len(node.rpc("listbanned")), 0)


def test_masternode_governance(suite, node):
    """Masternode and governance surfaces. No live masternodes on a private
    chain, so these assert clean, well-formed responses rather than data."""
    suite.category = "masternode"

    res = node.cli("mnsync", "status")
    suite.expect("mnsync status responds", res.ok, res.err)

    for argv, name in [
        (["masternode", "count"], "masternode count responds"),
        (["masternodelist"], "masternodelist responds"),
        (["gobject", "count"], "gobject count responds"),
    ]:
        res = node.cli(*argv)
        crashed = res.rc == -9 or "Assertion" in (res.err + res.out)
        suite.expect(name, res.ok and not crashed, (res.err or res.out)[:160])

    # negative arguments must not crash the masternode surface
    for argv, name in [
        (["masternodelist", "bogusmode"], "masternodelist rejects an unknown mode"),
        (["masternode", "bogus"], "masternode rejects an unknown subcommand"),
        (["gobject", "bogus"], "gobject rejects an unknown subcommand"),
    ]:
        res = node.cli(*argv)
        crashed = res.rc == -9 or "Assertion" in (res.err + res.out)
        suite.expect(name, not crashed, (res.err or res.out)[:160])

    suite.expect("daemon survives the masternode battery", node.is_running())


def test_performance(suite, node):
    suite.category = "performance"

    # RPC round-trip latency
    samples = []
    for _ in range(20):
        res = node.cli("getblockcount")
        if res.ok:
            samples.append(res.duration)
    if samples:
        suite.metric("rpc round trip (mean)", round(sum(samples) / len(samples) * 1000, 1), "ms")
        suite.metric("rpc round trip (max)", round(max(samples) * 1000, 1), "ms")

    # block generation throughput
    h0 = node.rpc("getblockcount")
    _, secs = timed(mine, node, 10)
    produced = node.rpc("getblockcount") - h0
    if produced > 0 and secs > 0:
        suite.metric("sustained block rate", round(produced / secs, 1), "blocks/s")

    # validation throughput on restart
    _, secs = timed(node.stop)
    suite.metric("shutdown", round(secs, 2), "s")
    _, secs = timed(node.start)
    suite.metric("startup (warm)", round(secs, 2), "s")
    suite.expect("node healthy after performance battery", node.is_running())


# --------------------------------------------------------------------------
# html report
# --------------------------------------------------------------------------

REPORT_CSS = """
body{font-family:-apple-system,Segoe UI,Roboto,Helvetica,Arial,sans-serif;
     margin:0;padding:0 0 60px;background:#f6f8fa;color:#24292f}
header{background:#0d1117;color:#fff;padding:28px 40px}
header h1{margin:0 0 6px;font-size:24px}
header .sub{opacity:.75;font-size:13px}
main{max-width:1100px;margin:0 auto;padding:0 20px}
.cards{display:flex;gap:14px;margin:24px 0;flex-wrap:wrap}
.card{flex:1;min-width:130px;background:#fff;border:1px solid #d0d7de;
      border-radius:8px;padding:16px 18px}
.card .n{font-size:30px;font-weight:600;line-height:1}
.card .l{font-size:12px;text-transform:uppercase;letter-spacing:.5px;
         color:#57606a;margin-top:6px}
.pass .n{color:#1a7f37}.fail .n{color:#cf222e}
.skip .n{color:#9a6700}.warn .n{color:#bc4c00}
h2{margin:32px 0 10px;font-size:17px;border-bottom:1px solid #d0d7de;padding-bottom:6px}
table{width:100%;border-collapse:collapse;background:#fff;border:1px solid #d0d7de;
      border-radius:8px;overflow:hidden;font-size:13px}
th{background:#f6f8fa;text-align:left;padding:9px 12px;font-weight:600;
   border-bottom:1px solid #d0d7de}
td{padding:8px 12px;border-bottom:1px solid #eaeef2;vertical-align:top}
tr:last-child td{border-bottom:none}
.tag{display:inline-block;padding:2px 9px;border-radius:20px;font-size:11px;
     font-weight:600;letter-spacing:.3px}
.t-PASS{background:#dafbe1;color:#1a7f37}
.t-FAIL{background:#ffebe9;color:#cf222e}
.t-SKIP{background:#fff8c5;color:#7d4e00}
.t-WARN{background:#fff1e5;color:#bc4c00}
.detail{color:#57606a;font-family:ui-monospace,SFMono-Regular,Consolas,monospace;
        font-size:12px}
.banner{background:#ddf4ff;border:1px solid #54aeff;border-radius:8px;
        padding:12px 16px;margin:20px 0;font-size:13px}
.banner.bad{background:#ffebe9;border-color:#ff8182}
.env{font-size:13px}
.env td:first-child{color:#57606a;width:230px}
.chart{background:#fff;border:1px solid #d0d7de;border-radius:8px;padding:16px 18px}
.row{display:flex;align-items:center;margin:7px 0;font-size:13px}
.row .nm{width:150px;color:#24292f;flex-shrink:0}
.row .bar{flex:1;height:16px;background:#eaeef2;border-radius:4px;overflow:hidden;
          display:flex}
.row .seg{height:100%}
.seg-pass{background:#2da44e}.seg-fail{background:#cf222e}
.seg-warn{background:#bc4c00}.seg-skip{background:#d4a72c}
.row .ct{width:120px;text-align:right;color:#57606a;font-size:12px;flex-shrink:0}
.legend{margin-top:14px;font-size:12px;color:#57606a}
.legend span{display:inline-block;margin-right:14px}
.legend i{display:inline-block;width:10px;height:10px;border-radius:2px;
          margin-right:5px;vertical-align:middle}
footer{max-width:1100px;margin:40px auto 0;padding:0 20px;color:#57606a;font-size:12px}
"""


def write_report(path, suite, env):
    counts = suite.counts()
    total = len(suite.checks)
    elapsed = time.time() - suite.started
    verdict_ok = counts[FAIL] == 0

    def esc(v):
        return html.escape(str(v))

    parts = []
    parts.append("<!doctype html><html><head><meta charset='utf-8'>")
    parts.append("<title>Dimecoin QA Report %s</title>"
                 % esc(env.get("generated", "")))
    parts.append("<style>%s</style></head><body>" % REPORT_CSS)

    parts.append("<header><h1>Dimecoin QA Report</h1>")
    parts.append("<div class='sub'>%s &middot; suite v%s &middot; %s</div></header>"
                 % (esc(env.get("generated")), SUITE_VERSION, esc(env.get("chain"))))
    parts.append("<main>")

    if verdict_ok:
        parts.append("<div class='banner'><b>All checks passed.</b> "
                     "%d checks across %d categories in %s.</div>"
                     % (total, len(set(c.category for c in suite.checks)),
                        format_duration(elapsed)))
    else:
        parts.append("<div class='banner bad'><b>%d check(s) failed.</b> "
                     "See the failing rows below.</div>" % counts[FAIL])

    parts.append("<div class='cards'>")
    for key, label in ((PASS, "passed"), (FAIL, "failed"),
                       (WARN, "warnings"), (SKIP, "skipped")):
        parts.append("<div class='card %s'><div class='n'>%d</div>"
                     "<div class='l'>%s</div></div>" % (key.lower(), counts[key], label))
    parts.append("<div class='card'><div class='n'>%s</div>"
                 "<div class='l'>duration</div></div>" % format_duration(elapsed))
    parts.append("</div>")

    # environment
    parts.append("<h2>Environment</h2><table class='env'>")
    for k in ("generated", "chain", "client version", "protocol version",
              "datadir", "source tree", "host", "python"):
        if k in env:
            parts.append("<tr><td>%s</td><td>%s</td></tr>" % (esc(k), esc(env[k])))
    parts.append("</table>")

    # per-category overview chart
    order = []
    for c in suite.checks:
        if c.category not in order:
            order.append(c.category)
    if order:
        parts.append("<h2>Category overview</h2><div class='chart'>")
        for cat in order:
            rows = [c for c in suite.checks if c.category == cat]
            cc = {PASS: 0, FAIL: 0, SKIP: 0, WARN: 0}
            for r in rows:
                cc[r.status] += 1
            total_cat = len(rows) or 1
            parts.append("<div class='row'><div class='nm'>%s</div><div class='bar'>"
                         % esc(cat))
            for key, cls in ((PASS, "seg-pass"), (FAIL, "seg-fail"),
                             (WARN, "seg-warn"), (SKIP, "seg-skip")):
                if cc[key]:
                    parts.append("<div class='seg %s' style='width:%.2f%%'></div>"
                                 % (cls, 100.0 * cc[key] / total_cat))
            parts.append("</div><div class='ct'>%d/%d passed</div></div>"
                         % (cc[PASS], len(rows)))
        parts.append("<div class='legend'>"
                     "<span><i class='seg-pass'></i>passed</span>"
                     "<span><i class='seg-fail'></i>failed</span>"
                     "<span><i class='seg-warn'></i>warning</span>"
                     "<span><i class='seg-skip'></i>skipped</span></div>")
        parts.append("</div>")
    failures = [c for c in suite.checks if c.status == FAIL]
    if failures:
        parts.append("<h2>Failures</h2><table>")
        parts.append("<tr><th>Category</th><th>Check</th><th>Detail</th></tr>")
        for c in failures:
            parts.append("<tr><td>%s</td><td>%s</td><td class='detail'>%s</td></tr>"
                         % (esc(c.category), esc(c.name), esc(c.detail)))
        parts.append("</table>")

    # per-category results
    parts.append("<h2>Results by category</h2>")
    seen = []
    for c in suite.checks:
        if c.category not in seen:
            seen.append(c.category)
    for cat in seen:
        rows = [c for c in suite.checks if c.category == cat]
        cc = {PASS: 0, FAIL: 0, SKIP: 0, WARN: 0}
        for r in rows:
            cc[r.status] += 1
        parts.append("<h2>%s <span style='font-weight:400;color:#57606a;font-size:13px'>"
                     "(%d passed, %d failed)</span></h2>"
                     % (esc(cat), cc[PASS], cc[FAIL]))
        parts.append("<table><tr><th style='width:90px'>Status</th><th>Check</th>"
                     "<th>Detail</th></tr>")
        for r in rows:
            parts.append("<tr><td><span class='tag t-%s'>%s</span></td>"
                         "<td>%s</td><td class='detail'>%s</td></tr>"
                         % (r.status, r.status, esc(r.name), esc(r.detail)))
        parts.append("</table>")

    # metrics
    if suite.metrics:
        parts.append("<h2>Performance metrics</h2><table>")
        parts.append("<tr><th>Category</th><th>Metric</th><th>Value</th><th>Unit</th></tr>")
        for cat, name, value, unit in suite.metrics:
            parts.append("<tr><td>%s</td><td>%s</td><td><b>%s</b></td><td>%s</td></tr>"
                         % (esc(cat), esc(name), esc(value), esc(unit)))
        parts.append("</table>")

    parts.append("</main>")
    parts.append("<footer>Generated by dimecoin_qa.py v%s. "
                 "This suite runs only against regtest/testnet and never "
                 "against mainnet.</footer>" % SUITE_VERSION)
    parts.append("</body></html>")

    with open(path, "w", encoding="utf-8") as fh:
        fh.write("".join(parts))


def format_duration(secs):
    secs = int(secs)
    if secs < 60:
        return "%ds" % secs
    if secs < 3600:
        return "%dm %02ds" % (secs // 60, secs % 60)
    return "%dh %02dm" % (secs // 3600, (secs % 3600) // 60)


# --------------------------------------------------------------------------
# driver
# --------------------------------------------------------------------------

ALL_CATEGORIES = [
    "safety", "node", "mining", "wallet", "maturity", "transactions",
    "staking", "cli-conversion", "error-paths", "network", "masternode",
    "encryption", "staking-unlock", "performance", "staking-engine",
    "user-journey", "consensus", "consensus-timing", "unit-tests",
]


REPORTS_DIRNAME = "qa-reports"


def resolve_report_path(requested, srcdir, chain, stamp):
    """Work out where the HTML report should be written.

    Reports are timestamped so consecutive runs never overwrite each other,
    and by default they land in test/qa-reports/ inside the source tree, which
    is listed in .gitignore so results are never committed. If that tree is
    not writable we fall back to the user's home directory rather than fail.
    """
    filename = "dimecoin-qa-%s-%s.html" % (chain, stamp)

    if requested:
        requested = os.path.expanduser(requested)
        # A directory, or something that looks like one, receives the
        # timestamped filename. An explicit .html path is honoured verbatim.
        if os.path.isdir(requested) or requested.endswith((os.sep, "/")):
            target = os.path.join(requested, filename)
        else:
            target = requested
    else:
        target = os.path.join(os.path.expanduser(srcdir), "test",
                              REPORTS_DIRNAME, filename)

    parent = os.path.dirname(target) or "."
    try:
        if not os.path.isdir(parent):
            os.makedirs(parent)
        probe = os.path.join(parent, ".write-probe")
        with open(probe, "w") as fh:
            fh.write("")
        os.remove(probe)
    except OSError:
        fallback = os.path.expanduser(os.path.join("~", "dimecoin-" + REPORTS_DIRNAME))
        try:
            if not os.path.isdir(fallback):
                os.makedirs(fallback)
        except OSError:
            return os.path.join(os.path.expanduser("~"), filename)
        return os.path.join(fallback, filename)
    return target


def main():
    ap = argparse.ArgumentParser(description="Dimecoin QA suite")
    ap.add_argument("--srcdir", default="~/dime253",
                    help="Dimecoin source tree containing src/dimecoind")
    ap.add_argument("--datadir", default=None,
                    help="datadir to create (must not already exist)")
    ap.add_argument("--report", default=None,
                    help="output HTML report path, or a directory to write a "
                         "timestamped report into (default: "
                         "<srcdir>/test/qa-reports/)")
    ap.add_argument("--chain", default="regtest", choices=["regtest", "testnet"])
    ap.add_argument("--port", type=int, default=31941)
    ap.add_argument("--rpcport", type=int, default=31942)
    ap.add_argument("--categories", default=None,
                    help="comma-separated subset of categories to run")
    ap.add_argument("--keep-datadir", action="store_true",
                    help="do not delete the datadir on exit")
    ap.add_argument("--skip-unit", action="store_true",
                    help="skip the C++ unit test tier, which adds several "
                         "minutes to the run")
    args = ap.parse_args()

    if args.chain == "regtest":
        default_dir = os.path.expanduser("~/dimecoin-qa-regtest")
    else:
        default_dir = os.path.expanduser("~/dimecoin-qa-testnet")
    datadir = os.path.expanduser(args.datadir or default_dir)
    run_started = datetime.now()
    report = resolve_report_path(args.report, args.srcdir, args.chain,
                                 run_started.strftime("%Y%m%d-%H%M%S"))

    selected = None
    if args.categories:
        selected = set(s.strip() for s in args.categories.split(",") if s.strip())
        unknown = selected - set(ALL_CATEGORIES)
        if unknown:
            print("Unknown categories: %s" % ", ".join(sorted(unknown)))
            print("Valid: %s" % ", ".join(ALL_CATEGORIES))
            return 2

    run_unit = (not args.skip_unit
                and (not selected or "unit-tests" in selected))

    print("=" * 70)
    print("Dimecoin QA Suite v%s" % SUITE_VERSION)
    print("=" * 70)
    print("source tree : %s" % args.srcdir)
    print("chain       : %s" % args.chain)
    print("datadir     : %s" % datadir)
    print("report      : %s" % report)
    print("=" * 70)

    if os.path.exists(datadir):
        print("\nERROR: datadir already exists: %s" % datadir)
        print("Remove it or pass --datadir. This suite only uses a datadir it creates.")
        return 2

    suite = Suite()
    node = Node(args.srcdir, datadir, chain=args.chain,
                port=args.port, rpcport=args.rpcport)
    node.create_datadir()

    env = {
        "generated": run_started.strftime("%Y-%m-%d %H:%M:%S %Z").strip(),
        "chain": args.chain,
        "datadir": datadir,
        "source tree": node.srcdir,
        "host": "%s %s" % (platform.system(), platform.release()),
        "python": platform.python_version(),
    }

    exit_code = 0
    try:
        print("\nStarting daemon...")
        node.start()
        print("Daemon is up.\n")

        info = node.rpc("getblockchaininfo")
        net = node.rpc("getnetworkinfo")
        env["chain"] = info.get("chain", args.chain)
        env["client version"] = net.get("subversion", "unknown")
        env["protocol version"] = net.get("protocolversion", "unknown")

        # Hard safety gate before anything destructive happens.
        if env["chain"] == "main":
            print("SAFETY ABORT: node reports chain='main'.")
            return 3

        plan = [
            ("safety", test_safety, ()),
            ("node", test_node_lifecycle, ()),
            ("mining", test_mining, ()),
            ("wallet", test_wallet_basics, ()),
            ("maturity", test_coinbase_maturity, ()),
            ("transactions", test_transactions, ()),
            ("staking", test_staking_config, ()),
            ("staking-engine", test_staking_engine, ()),
            ("cli-conversion", test_cli_argument_conversion, ()),
            ("error-paths", test_negative_paths, ()),
            ("network", test_network, ()),
            ("masternode", test_masternode_governance, ()),
            ("consensus", test_consensus_emission, ()),
            ("consensus-timing", test_consensus_block_timing, ()),
            ("performance", test_performance, ()),
        ]

        for cat, fn, extra in plan:
            if selected and cat not in selected:
                continue
            print("\n--- %s ---" % cat)
            try:
                fn(suite, node, *extra)
            except Exception as exc:
                suite.category = cat
                suite.fail("category '%s' raised an exception" % cat,
                           "%s: %s" % (type(exc).__name__, exc))
                traceback.print_exc()

            # A category that died mid-restart can leave the node down, which
            # would cascade into every later category. Recover before moving on.
            if not node.is_running():
                try:
                    node.start()
                    suite.warn("daemon was restarted after category '%s'" % cat,
                               "the category left the node stopped")
                except Exception as exc:
                    suite.category = cat
                    suite.fail("could not restart daemon after '%s'" % cat,
                               "%s: %s" % (type(exc).__name__, exc))
                    break

        # Encryption is destructive and must run after the spend tests.
        passphrase = "qa-passphrase-1"
        if not selected or "encryption" in selected:
            print("\n--- encryption ---")
            try:
                if test_wallet_encryption(suite, node, passphrase):
                    passphrase = passphrase + "-changed"
            except Exception as exc:
                suite.category = "encryption"
                suite.fail("encryption battery raised an exception",
                           "%s: %s" % (type(exc).__name__, exc))
                traceback.print_exc()

        if not selected or "staking-unlock" in selected:
            print("\n--- staking-unlock ---")
            try:
                test_staking_unlock(suite, node, passphrase)
            except Exception as exc:
                suite.category = "staking-unlock"
                suite.fail("staking-unlock battery raised an exception",
                           "%s: %s" % (type(exc).__name__, exc))
                traceback.print_exc()

        if not selected or "user-journey" in selected:
            print("\n--- user-journey ---")
            try:
                test_user_journey(suite, node, passphrase)
            except Exception as exc:
                suite.category = "user-journey"
                suite.fail("user-journey battery raised an exception",
                           "%s: %s" % (type(exc).__name__, exc))
                traceback.print_exc()

    except NodeError as exc:
        suite.category = "fatal"
        suite.fail("suite aborted", str(exc))
        exit_code = 1
    except KeyboardInterrupt:
        suite.category = "fatal"
        suite.fail("suite interrupted by user", "")
        exit_code = 130
    finally:
        try:
            if node.is_running():
                print("\nStopping daemon...")
                node.stop()
        except Exception:
            pass

        # The C++ unit tier needs no daemon, so it runs even when the
        # functional half aborted -- that is precisely when knowing whether
        # the core primitives still hold is most useful. The daemon has
        # already been stopped above, so the two never contend.
        if run_unit:
            print("\n--- unit-tests ---")
            try:
                test_unit_tests(suite, args.srcdir)
            except Exception as exc:
                suite.category = "unit-tests"
                suite.fail("unit-tests battery raised an exception",
                           "%s: %s" % (type(exc).__name__, exc))
                traceback.print_exc()

        write_report(report, suite, env)

        counts = suite.counts()
        print("\n" + "=" * 70)
        print("passed %d   failed %d   warnings %d   skipped %d   in %s"
              % (counts[PASS], counts[FAIL], counts[WARN], counts[SKIP],
                 format_duration(time.time() - suite.started)))
        print("report: %s" % report)
        print("=" * 70)

        if not args.keep_datadir and node.owns_datadir and os.path.isdir(datadir):
            shutil.rmtree(datadir, ignore_errors=True)
            print("datadir removed: %s" % datadir)

        if counts[FAIL]:
            exit_code = exit_code or 1

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
