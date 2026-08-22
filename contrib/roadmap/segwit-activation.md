# Phase 1.1 — Activate SegWit (soft fork)

## Current state (verified)

- Deployment window is **expired** in `src/chainparams.cpp`:
  - mainnet `:127-129` — `nStartTime = 1479168000` (2016-11-15), `nTimeout = 1510704000` (2017-11-15)
  - testnet `:260-262`, regtest `:380-382` — same expired window
- Enforcement code **is present**: `IsWitnessEnabled(consensusParams)` is used throughout `src/net_processing.cpp` (compact-block witness handling), so once the deployment locks in, segwit rules enforce automatically.
- Result today: segwit never reached signalling threshold in the old window, so `DEPLOYMENT_SEGWIT` is neither locked-in nor active.

## What to change

Open a fresh BIP9 window and coordinate signalling. The change is tiny and mergeable; the _activation_ itself is a network event (miners signal).

### Draft diff (`src/chainparams.cpp`, mainnet block at `:127-129`)

```diff
         consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
-        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
-        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.
+        // Revival soft-fork — set dates ONLY after community + miner signalling agreement.
+        // Example (EDIT): start ~6 months out, timeout +1 year.
+        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1800000000; // ~2027-01-15 UTC (TODO)
+        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout  = 1831536000; // ~2028-01-15 UTC (TODO)
```

Mirror the same edit on testnet (`:264-266`) and regtest (`:391-393`) as appropriate (regtest usually uses immediate/always-active for testing).

## Activation procedure (team runs)

1. **Decide threshold**: Bitcoin used 95% over 8064 blocks. For a small chain, consider a lower threshold or a flag-day / mandatory activation (BIP148-style user-activated) to avoid failing to signal.
2. **Announce**: publish the `nStartTime` so miners/pools upgrade `dimecoind` before the window opens.
3. **Signal**: miners set `VERSIONBITS_TOP_BITS` signalling bit 1; once threshold met within a window, segwit locks in, then enforces after the retarget.
4. **Verify**: after lock-in, `getblockchaininfo` → `bip9_softforks.segwit.status == "active"`; `IsWitnessEnabled` returns true; bech32 (`vx`) addresses become spendable.
5. **Wallet**: once active, expose `vx`-prefixed addresses and witness transactions in the RPC/CLI.

## Why it matters

- bech32 (`vx`) native addresses (smaller fees, copy-paste safety)
- Enables **Lightning Network** (Phase 3.2)
- Compatible with modern wallet libraries (most assume segwit)

## Risks

- Soft fork only if >50% hashrate enforces — coordinate with the (Quark) mining community first.
- If miners don't upgrade before `nStartTime`, blocks may be rejected post-lock-in. Staggered rollout + clear comms mitigates this.

## Appendix — Taproot deployment entry (Phase 1.2, depends on PR #96 Schnorr)

After Schnorr/Taproot validation lands, add (example, mainnet):

```cpp
consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1900000000; // TODO
consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout  = 1931536000; // TODO
```

Requires `SCRIPT_VERIFY_TAPROOT` + tapscript interpreter in `src/script/interpreter.cpp` (not yet present — track separately).
