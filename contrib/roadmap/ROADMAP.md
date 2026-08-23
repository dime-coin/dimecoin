# Dimecoin Revival Enablement — Draft Roadmap (community)

> **Status:** community draft. The maintainers (`dime-coin`) will publish the
> official Dimecoin roadmap; this document is a starting point for discussion,
> not a merged plan. All code pointers are to `dime-coin/dimecoin` branch
> `2.4.0.0` as of 2026-08-22.
>
> **Base fact correction:** Dimecoin `2.0.0.0` was rebased over **Bitcoin Core
> 0.17** (not 0.13/0.14). Early drafts inferred 0.13/0.14 from the CSV timeout
> (1493596800 = May 2017); that inference was wrong. Keep this in mind for any
> rebase/modernization estimates.

## Phase 0 — Stabilization & consensus hardening (do first)

Before adding features, harden what exists:

- Audit hybrid **PoW (Quark) / PoS** consensus, coinstake handling, block
  signatures, masternode & foundation payments.
- Confirm chainparams (ports, prefixes, genesis) match the live network; remove
  any copy-paste Bitcoin/Dash leftovers.
- Review seed-node lists (`contrib/seeds/*`) — stale/foreign entries cause
  sync issues. (See PR #86.)
- Pin down what is and is not enforced today (SegWit framework, sporks) so later
  phases build on verified state, not assumptions.

## Phase 1 — Protocol modernization

### 1.1 Activate SegWit (soft fork) — its own proposal

- **SegWit was NEVER activated on Dimecoin.** The BIP9 deployment window in
  `src/chainparams.cpp` (`DEPLOYMENT_SEGWIT`, mainnet `:127-129`, testnet
  `:260-262`, regtest `:380-382`) expired without reaching threshold, so the
  feature is neither locked-in nor active. The witness _framework_ (e.g.
  `IsWitnessEnabled`) is inherited from the Bitcoin 0.17 rebase but is dormant.
- This is **not** a one-line re-enable. Activation must be designed against
  Dimecoin's specifics: hybrid PoW/PoS, coinstake transactions, block
  signatures, witness commitments, masternode/foundation payment outputs,
  wallet handling, and **versionbits signalling for both miners AND stakers**.
  Treat it as its own design proposal (draft: `segwit-activation.md`) with
  community + miner/staker sign-off before any date is set.
- Enables: bech32 (`vx`) addresses, modern wallets. (Lightning is a separate,
  longer-term concern — see 3.2.)

### 1.2 Taproot (BIP340/341/342) — longer term

- Depends on `secp256k1` Schnorr (PR #96). #96 is useful groundwork but is
  currently **experimental and unused** (no in-tree feature requires it yet, and
  it lacks EllSwift for BIP324). Taproot scripts/tapscript + validation are
  still to land and should follow a reviewed feature that needs them.
- Not "mostly wired" — keep it in the longer-term bucket.

### 1.3 Deterministic (DIP3-style) masternodes — concrete planned goal

- Dimecoin already has a Dash-derived masternode layer. Moving to
  **deterministic, DIP3-style masternodes** (on-chain registered,
  deterministic payouts) is a concrete planned objective and deserves its own
  major phase, separate from spork/governance plumbing.
- Draft the migration (registration tx, list enforcement, payout logic) as a
  standalone proposal.

### 1.4 Sporks & governance — separate track

- Keep spork keys / governance **separate** from the masternode work above.
- Status: single `strSporkPubKey` (`src/chainparams.cpp:177,307`) — centralized
  control risk. Options: move spork keys to a community multi-sig, or a
  transparent treasury/DAO model. **Multisig/DAO is not yet agreed** — scope it
  with the community; do not assume it in the plan.

### 1.5 Re-evaluate Quark PoW / network security

- Status: `HashQuark` (`src/hash.h`) — 6-round
  blake/bmw/groestl/jh/keccak/skein.
- Frame this as **"evaluate PoW / network security"**, not as a pre-chosen
  migration. Do not list yespower or SHA256 merge-mining as candidates without a
  deeper rationale. Any change alters consensus → community vote.

## Phase 2 — Wallet & UX

### 2.1 Electrum-DIME light wallet

- Dimecoin already has purpose-built Electrum infrastructure:
  [`dime-coin/electrum-dimecoin`](https://github.com/dime-coin/electrum-dimecoin)
  and
  [`dime-coin/electrumx-dimecoin`](https://github.com/dime-coin/electrumx-dimecoin).
  **Do not fork generic Electrum/ElectrumX** — use the Dimecoin-specific repos.
- BIP158 block filters (PR #97) + BIP157 filter index (PR #98) provide a
  separate **Neutrino** light-client path that needs no ElectrumX.
- Note: Dimecoin's transaction version 2 carries a Dimecoin-specific
  `strTxComment` serialization, so the backend must handle Dimecoin tx logic
  (a Dash/Bitcoin clone will not).

### 2.2 Hardware wallet registration (Trezor / Ledger / SLIP44)

- Dimecoin is **hybrid PoW/PoS with masternodes** — not "PoW Quark" alone.
  Correct any doc that simplifies it to PoW only.
- DIME has **no registered SLIP44 coin type** yet — request one
  (satoshilabs/slips) before submission. bech32 (`vx`) is only relevant
  **after** SegWit activates (1.1). See `hardware-wallet.md`.

### 2.3 PSBT multisig

- PSBT (BIP174) is already in core. Document + provide a sign/CLI example; pair
  with hardware wallet support above.

## Phase 3 — Interop & ecosystem (individual efforts)

### 3.1 DEX integrations (in flight, community efforts)

- These are **individual integration efforts**, not validated deliverables.
- AtomicDEX: DIME already tradeable (`mm2:1`); reference config in
  `contrib/dex/dimecoin-atomicdex.json`.
- BasicSwap: needs an upstream coin-interface PR
  (`contrib/dex/dimecoin-basicswap.json` is the reference).
- DCRDEX: blueprint `contrib/dex/dcrdex.md`.
- THORChain/Maya: blueprint `contrib/dex/thorchain-maya.md` — **heavy /
  research**, external; not a near-term deliverable.

### 3.2 Lightning Network — research / future

- Move Lightning out of the Phase 3 deliverables into research/future. It
  depends on SegWit (1.1) and a reviewed, maintained Dimecoin backend; do not
  present it as a near-term item.

### 3.3 Explorer / docs / SDK

- Modernize the existing explorer; refresh README; define what "publish
  developer SDK" means before committing to it. Avoid referencing specific live
  chain heights in the roadmap (they go stale).

## Biggest risk

Rebase onto a modern Bitcoin Core. Dimecoin `2.0.0.0` already re-based over
**Bitcoin Core 0.17**; further modernization is a large effort and is a
prerequisite for cleanly landing Taproot/Lightning. Track separately, after
Phase 0 stabilization.
