# Dimecoin Revival Enablement — Phases 1–3

Team-ready plan + drafts so the only remaining work is review, signalling, and running.
All code pointers are to `dime-coin/dimecoin` branch `2.4.0.0` as of 2026-08-22.

## Phase 1 — Protocol modernization

### 1.1 Activate SegWit (soft fork)

- Status: deployment window expired (`src/chainparams.cpp:127-129`, start Nov 2016 / timeout Nov 2017). Enforcement code IS present (`IsWitnessEnabled` in `src/net_processing.cpp`).
- Action: set a fresh BIP9 window + coordinate miner signalling. Draft + exact diff: `segwit-activation.md`.
- Enables: bech32 (`vx`) addresses, Lightning, modern wallets.

### 1.2 Complete Taproot (BIP340/341/342)

- Depends on `secp256k1-enable-schnorr` (PR #96) — Schnorr already wired; Taproot scripts/tapscript + validation still to land.
- Action: implement `SCRIPT_VERIFY_TAPROOT`, tapscript interpreter, `DEPLOYMENT_TAPROOT` entry (draft in `segwit-activation.md` appendix).
- Enables: cheaper multisig, better privacy, DLCs.

### 1.3 Modernize the Dash masternode / spork layer

- Status: single `strSporkPubKey` (`src/chainparams.cpp:177,307`) — centralized control risk.
- Action: move spork keys to a community multi-sig; or replace spork/governance with a transparent treasury/DAO (Dash-proposals style). Draft: see roadmap note; detailed spec TBD with community.

### 1.4 Re-evaluate Quark PoW

- Status: `HashQuark` (`src/hash.h:240`) — 6-round blake/bmw/groestl/jh/keccak/skein.
- Options: (a) keep for continuity; (b) migrate to ASIC-resistant (yespower) via hard fork; (c) merge-mine with a larger SHA256 chain.
- Decision changes consensus → requires community vote. Leave for Phase 4 governance.

## Phase 2 — Wallet & UX

### 2.1 Electrum-DIME light wallet

- Status: BIP158 block filters submitted (PR #97 BIP158 filter + #98 BIP157 filter index), in review. `contrib/dex/electrumx/` provides a server.
- Action: fork Electrum (or ElectrumX-backed light client) with Dimecoin coin params; spec: `light-wallet.md`.
- Enables: mobile/desktop wallet with no full node.

### 2.2 Hardware wallet registration (Trezor / Ledger)

- Status: not registered.
- Action: submit SLIP44 coin type request + coin params (pub 15 / p2sh 9 / wif 143 / hrp `vx`). Ready-to-send package: `hardware-wallet.md`.

### 2.3 PSBT multisig

- Status: PSBT (BIP174) already in core.
- Action: document + provide a sign/cli example; pair with hardware wallet above.

## Phase 3 — Interop & ecosystem

### 3.1 Land DEX integrations (in flight)

- AtomicDEX: live (`GLEECBTC/coins`, DIME `mm2:1`).
- BasicSwap: PR tecnovert/basicswap#3 (DIME interface).
- DCRDEX: upstream issue decred/dcrdex#3626 + blueprint `contrib/dex/dcrdex.md`.
- THORChain/Maya: blueprint `contrib/dex/thorchain-maya.md` (external, heavy).
- Komodo coin def rpcport fix: GLEECBTC/coins#1960.

### 3.2 Lightning Network

- Action: after 1.1 (SegWit) — add `lnd`/`c-lightning` Dimecoin chain config (reuse Bitcoin plugin + Dimecoin params).

### 3.3 Explorer / docs / SDK

- Action: modernize explorer (currently serving fine, height ~7.45M), publish developer SDK, refresh README.

## Biggest risk

Rebase onto modern Bitcoin Core (current base ~0.13/0.14, inferred from CSV timeout 1493596800 = May 2017). 6–12 months; prerequisite for Taproot/Lightning cleanly. Track in Phase 4.
