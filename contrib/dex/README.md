# Dimecoin on non-custodial DEXs

This directory packages the protocol constants Dimecoin needs to be listed on
non-custodial (atomic-swap) DEXs, plus a runbook for the team. The goal is:
the team approves this PR and runs an ElectrumX server for redundancy - no
marketing, no custody, no listing fee.

## Status (verified 2026-08-22)

- **AtomicDEX (Komodo MM2): DIME is ALREADY tradeable.** DIME is in AtomicDEX's
  default coin list (`mm2: 1`, correct params) and the ElectrumX servers
  `electrumx1.dimecoinnetwork.com` / `electrumx2.dimecoinnetwork.com` are live
  (verified `ElectrumX 1.15.0` on `:50001`). The community can enable/swap DIME
  on AtomicDEX today. `dimecoin-atomicdex.json` is a copy-paste custom-enable
  config and a reference; no upstream submission is required to make AtomicDEX
  work.
- **BasicSwap (Particl): NOT supported yet.** BasicSwap supports DASH but not
  DIME. Enabling DIME needs an upstream coin-interface PR (steps below).
  `dimecoin-basicswap.json` is the chainparams reference for that PR.
- **THORChain / Maya, NEAR Intents, SideShift.ai, ChangeNOW, no-KYC swap
  aggregators:** already support DASH; DIME could be added by the same path but
  those are separate upstream projects (out of scope for this repo).

## Extracted protocol constants (from `src/chainparams.cpp`)

| Parameter              | Mainnet                                                            | Testnet | Regtest |
| ---------------------- | ------------------------------------------------------------------ | ------- | ------- |
| P2PKH base58 prefix    | 15 (0x0F)                                                          | 15      | 15      |
| P2SH base58 prefix     | 9 (0x09)                                                           | 9       | 9       |
| WIF prefix             | 143 (0x8F)                                                         | 143     | 143     |
| xpub / HD pub version  | 0x0488B21E (76067358)                                              | -       | -       |
| xprv / HD priv version | 0x0488ADE4 (76066276)                                              | -       | -       |
| bech32 HRP             | `vx`                                                               | -       | -       |
| P2P port               | 11931                                                              | 21931   | 31931   |
| RPC port               | **8332** (real default, `chainparamsbase.cpp:36`)                  | 18332   | 18332   |
| Transaction version    | 2                                                                  | 2       | 2       |
| PoW algorithm          | x11                                                                | x11     | x11     |
| Genesis block hash     | `00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da` |         |         |
| Message start          | `fea503dd`                                                         |         |         |

NOTE: some upstream coin lists show `rpcport: 11931` for DIME by mistake - that
is the P2P port, not the RPC port. Dimecoin's node RPC port is 8332.

Dimecoin reuses Bitcoin's xpub/xprv prefixes (0x0488B21E / 0x0488ADE4), so HD
key derivation is identical to Bitcoin.

Seed nodes: `seed1.dimecoinnetwork.com`, `seed2.dimecoinnetwork.com`,
`node1.dimecoinnetwork.com`, `node2.dimecoinnetwork.com`,
`dime-pool.dimecoinnetwork.com`.

## Config files in this directory

- `dimecoin-atomicdex.json` - AtomicDEX / KomodoPlatform MM2 coin config with
  the live ElectrumX servers filled in. Usable as a custom `enable` config.
- `dimecoin-basicswap.json` - BasicSwap chainparams reference (see BasicSwap
  steps below; needs an upstream interface PR).
- `electrumx/` - ready-to-run self-hosted ElectrumX setup (Docker Compose +
  configs) so the team can add redundant servers.

## 1. Run a self-hosted ElectrumX server (optional redundancy)

A single ElectrumX instance serves all SPV DEX clients. The `electrumx/`
directory contains a Docker Compose stack. Edit `electrumx/dimecoin.conf` with
the team's `dimecoind` rpc credentials, then:

```
cd contrib/dex/electrumx
docker compose up -d
```

It exposes TCP 10061 / SSL 20061 (WebSocket 30061) and connects to the local
`dimecoind -rpcport=8332`. ElectrumX already ships a Dimecoin-like (Bitcoin X11)
coin definition - if the packaged version lacks it, add one using the constants
table above (same shape as Dash). Publish the host as e.g.
`electrumx3.dimecoinnetwork.com:10061` and add it to `electrum_servers` in
`dimecoin-atomicdex.json` for extra redundancy.

## 2. Enable DIME on BasicSwap (upstream PR)

BasicSwap needs a coin interface. Using the DASH interface as the template:

1. Add `DIME = <n>` to the `Coins` enum in `basicswap/util.py`.
2. Create `basicswap/interface/dimecoin/chainparams.py` as a clone of
   `basicswap/interface/dash/chainparams.py` with the values from
   `dimecoin-basicswap.json` (pubkey_address 15, script_address 9, key_prefix
   143, hrp "vx", decimal_places 5, rpcport 8332).
3. Register it in `basicswap/chainparams.py` (`Coins.DIME: dimecoin_params`).
4. Open the PR to `tecnovert/basicswap` (or `particl/basicswap`).

`bip44: 15` in the JSON is a placeholder - confirm the real SLIP-44 coin type
before submitting.

## Why this is safe

- No code change to Dimecoin core; only config + an optional ElectrumX server
  the team already knows how to run.
- No KYC, no custody, no listing fee - consistent with the project's
  decentralization goals and with applicable promotion rules.
