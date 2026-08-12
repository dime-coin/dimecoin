# Dimecoin QA Suite

`functional/dimecoin_qa.py` is a single-entry test harness that exercises the
Dimecoin daemon end to end and writes a self-contained HTML report.

It is standalone. It does **not** use the inherited Bitcoin Core test framework
that also lives under `test/functional/` — that framework was never adapted for
Dimecoin and is not wired up.

---

## Safety

The suite is built so it cannot touch a production node:

* It **refuses to run against mainnet.** Before any destructive step it calls
  `getblockchaininfo` and aborts if `chain` is `main`.
* It **creates its own datadir** and refuses to start if that directory already
  exists, so it can never operate on a wallet you already own.
* It **uses its own ports** (default `31941`/`31942`, not the mainnet
  `11931`/`11932`) and fails fast with a clear message if they are occupied.
* It deletes the datadir it created when it finishes, unless you pass
  `--keep-datadir`.

Running it will never connect to, modify, or read your real wallet.

---

## Requirements

* A built Dimecoin source tree containing `src/dimecoind` and `src/dimecoin-cli`
* Python 3.6 or newer — **standard library only**, nothing to install

Optionally, `src/test/test_dimecoin` (built by `make check`). If it is present the
suite runs the C++ unit tier as well; if it is absent that tier is reported as
skipped and everything else still runs. Pass `--skip-unit` to skip it explicitly.

---

## Quick start

From the repository root, after a successful `make`:

```sh
cd test/functional
python3 dimecoin_qa.py --srcdir /path/to/dimecoin
```

That runs every category on a throwaway regtest chain and writes a timestamped
report into `test/qa-reports/` under the source tree, for example
`test/qa-reports/dimecoin-qa-regtest-20260730-195302.html`.

Reports are timestamped rather than overwritten, so consecutive runs can be
compared. `test/qa-reports/` is listed in `.gitignore`, so reports are never
committed.

A full run takes well under two minutes. Open the HTML file in any browser.

---

## Common invocations

Run everything and keep the report somewhere specific:

```sh
python3 dimecoin_qa.py --srcdir ~/dimecoin --report ~/qa-report.html
```

Run only the wallet and staking checks:

```sh
python3 dimecoin_qa.py --srcdir ~/dimecoin --categories wallet,staking,staking-unlock
```

Keep the chain afterwards so you can poke at it by hand:

```sh
python3 dimecoin_qa.py --srcdir ~/dimecoin --datadir ~/qa-chain --keep-datadir
```

Then:

```sh
~/dimecoin/src/dimecoin-cli -regtest -datadir=~/qa-chain \
    -rpcport=31942 -rpcuser=qauser -rpcpassword=qapass getblockchaininfo
```

Use different ports if the defaults clash with something you already run:

```sh
python3 dimecoin_qa.py --srcdir ~/dimecoin --port 41941 --rpcport 41942
```

Iterate quickly on functional tests only, skipping the slower C++ tier:

```sh
python3 dimecoin_qa.py --srcdir ~/dimecoin --skip-unit
```

---

## Options

| Option | Default | Meaning |
| --- | --- | --- |
| `--srcdir` | `~/dime253` | Source tree holding `src/dimecoind` |
| `--datadir` | `~/dimecoin-qa-regtest` | Datadir to create; must not already exist |
| `--report` | `<srcdir>/test/qa-reports/dimecoin-qa-<chain>-<timestamp>.html` | Where to write the HTML report |
| `--chain` | `regtest` | `regtest` or `testnet` |
| `--port` | `31941` | P2P port |
| `--rpcport` | `31942` | RPC port |
| `--categories` | all | Comma-separated subset to run |
| `--keep-datadir` | off | Do not delete the datadir on exit |
| `--skip-unit` | off | Skip the C++ unit tier and run only the functional tier |

Exit code is `0` when everything passed and non-zero when any check failed, so
it can be dropped straight into CI.

---

## What it covers

| Category | What it checks |
| --- | --- |
| `safety` | Not mainnet, isolated datadir, non-default ports |
| `node` | RPC surface, clean shutdown, restart preserves the chain |
| `mining` | `generate`, `generatetoaddress`, block/hash round-trips, chain tips |
| `wallet` | Address generation, validation, key export/import, backup, sign/verify |
| `maturity` | Coinbase rewards are immature, then become spendable |
| `transactions` | `sendtoaddress`, `sendmany`, mempool, confirmation, raw tx decode |
| `staking` | Split threshold, `listminting`, `-reservebalance` startup argument |
| `staking-engine` | Proves the minter starts, actively searches for a coinstake, and can be disabled |
| `cli-conversion` | Numeric/boolean arguments survive the `dimecoin-cli` conversion table |
| `error-paths` | Bad input produces a clean error rather than a crash or hang |
| `network` | Peer/ban RPCs, `setban` / `listbanned` / `clearbanned` |
| `masternode` | Masternode and governance RPCs respond and reject bad input |
| `consensus` | Block subsidy matches the emission model at each halving boundary and the supply stays within its cap |
| `consensus-timing` | Block spacing, difficulty retarget response, and the PoW/PoS cadence against the target block time |
| `encryption` | Encrypt, wrong passphrase rejected, locked wallet refuses keys and spends, passphrase change |
| `staking-unlock` | Unlock for staking only, still cannot spend, relocks correctly |
| `user-journey` | The full post-sync sequence: unlock, send, confirm, review, relock, unlock for staking, relock |
| `performance` | RPC latency, block rate, startup and shutdown timings |
| `unit-tests` | The full C++ Boost suite, folded into this report — see below |

The `cli-conversion` category exists because several real defects have lived
purely in `src/rpc/client.cpp`'s argument-conversion table. Those are invisible
to a JSON-RPC-only test, which is why this suite drives `dimecoin-cli` rather
than talking to the RPC port directly.

---

## The two tiers

Dimecoin has two separate bodies of tests, and this suite runs both from one
command.

**The C++ unit tier** (`src/test/test_dimecoin`) is inherited from Bitcoin Core
and built by `make check`. It runs compiled code in-process — no daemon, no
network, no wallet. It reaches internals that a CLI cannot see: script
evaluation, serialization, crypto primitives, subsidy and difficulty maths,
mempool ordering. It cannot see anything involving process startup, config
parsing, RPC wiring or persistence.

**The functional tier** (`dimecoin_qa.py`, everything else in this document)
starts a real `dimecoind` on a private chain and drives it over RPC exactly as a
user would. It proves whole-system behaviour, and it is the only tier that can
catch a defect which only appears once the daemon is actually running.

Neither replaces the other — each has found defects the other could not. So
`python3 dimecoin_qa.py` runs the functional tier, then invokes the unit binary,
parses its output, and folds the result into the same report and the same exit
code. There is no second command to remember. Use `--skip-unit` to opt out.

To run the unit tier by itself, without this harness:

```sh
cd src/test && ./test_dimecoin --log_level=test_suite
./test_dimecoin --run_test=miner_tests    # or any single suite
```

---

## Reading the report

The report opens with a pass/fail/warning/skip summary and a bar for each
category, so a regression is visible immediately. Below that:

* **Failures** — every failing check with its diagnostic, listed first
* **Results by category** — every check, grouped
* **Performance metrics** — timings and rates captured during the run

Statuses:

| Status | Meaning |
| --- | --- |
| `PASS` | The check succeeded |
| `FAIL` | A real problem; the run exits non-zero |
| `WARN` | Non-deterministic or environmental; look, but not necessarily a defect |
| `SKIP` | Not applicable to the selected chain |

---

## Troubleshooting

**`p2p port 31941 is already in use`**
Another daemon is running. Stop it, or pass `--port` / `--rpcport`.

**`datadir already exists`**
The suite never reuses a datadir. Delete it or pass a different `--datadir`.

**`binary not found: .../src/dimecoind`**
`--srcdir` is wrong, or the tree has not been built yet.

**`Authorization failed: Incorrect rpcuser or rpcpassword`**
A stale daemon is holding the RPC port. The suite normally catches this up
front; if you see it, check for a leftover `dimecoind` process.

---

## Known gaps

* **Stake *minting* is not asserted, only stake *attempting*.** The
  `staking-engine` category proves the minter thread starts, that the client
  repeatedly searches for a coinstake, that the wallet tracks stakeable
  outputs, and that `-staking=0` shuts it all down. It deliberately does not
  assert that a PoS block gets minted: winning a stake is probabilistic, so
  asserting it would make the suite flaky. Everything a user can observe and
  control about staking is covered.
* Masternode coverage asserts that the RPCs respond and reject bad input. A
  private chain has no live masternodes, so quorum behaviour is not exercised.
