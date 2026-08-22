# Dimecoin on non-custodial DEXs

This directory packages the protocol constants Dimecoin needs to be listed on
non-custodial (atomic-swap) DEXs, plus a runbook for the team. The goal is:
the team approves this PR, operates an ElectrumX server, and submits the coin
config upstream — no marketing, no custody, no listing fee.

Dimecoin is an X11 UTXO coin that is architecturally very close to Dash. Dash
is already integrated into AtomicDEX, BasicSwap and (as a live cross-chain
protocol) THORChain / Maya, so Dimecoin can follow the same integration paths
with a Dimecoin-specific config (below) plus one ElectrumX server.

## Extracted protocol constants (from `src/chainparams.cpp`)

| Parameter              | Mainnet                                                            | Testnet | Regtest |
| ---------------------- | ------------------------------------------------------------------ | ------- | ------- |
| P2PKH base58 prefix    | 15 (0x0F)                                                          | 15      | 15      |
| P2SH base58 prefix     | 9 (0x09)                                                           | 9       | 9       |
| WIF prefix             | 143 (0x8F)                                                         | 143     | 143     |
| xpub / HD pub version  | 0x0488B21E (76067358)                                              | —       | —       |
| xprv / HD priv version | 0x0488ADE4 (76066276)                                              | —       | —       |
| bech32 HRP             | `vx`                                                               | —       | —       |
| P2P port               | 11931                                                              | 21931   | 31931   |
| RPC port               | 8332                                                               | 18332   | 18332   |
| Transaction version    | 2                                                                  | 2       | 2       |
| PoW algorithm          | x11                                                                | x11     | x11     |
| Genesis block hash     | `00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da` |         |         |
| Message start          | `fea503dd`                                                         |         |         |

Note: Dimecoin reuses Bitcoin's xpub/xprv prefixes (0x0488B21E / 0x0488ADE4),
so HD key derivation is identical to Bitcoin.

Seed nodes: `seed1.dimecoinnetwork.com`, `seed2.dimecoinnetwork.com`,
`node1.dimecoinnetwork.com`, `node2.dimecoinnetwork.com`,
`dime-pool.dimecoinnetwork.com`.

## Config files in this directory

- `dimecoin-atomicdex.json` — coin definition for AtomicDEX / KomodoPlatform MM2
  (mirrors `electrums/DASH`). Submit to the MM2 coin list and to
  `KomodoPlatform/coins` as `electrums/DIME`.
- `dimecoin-basicswap.json` — coin definition for BasicSwap. Submit to
  `basicswap/basicswap` as `coins/dimecoin.json`.

Both files leave `electrum_servers` empty on purpose — fill it once the
team's ElectrumX server is up (see below).

## 1. Run an ElectrumX server (team operates this)

A single ElectrumX instance is enough for all the DEXs above to serve Dimecoin
SPV clients. Example `electrumx.conf`:

```
COIN = Dimecoin
DB_DIRECTORY = /var/electrumx/db
DAEMON_URL = http://user:password@127.0.0.1:8332/
PEER_DISCOVERY = on
HOST = 0.0.0.0
TCP_PORT = 10061
SSL_PORT = 20061
RPC_PORT = 8000
BANNER_FILE = /var/electrumx/banner
```

`DAEMON_URL` points at the team's `dimecoind -rpcport=8332` (with an rpc user/
password). The `COIN = Dimecoin` value requires ElectrumX to know the Dimecoin
coin parameters — if the packaged ElectrumX lacks a Dimecoin entry, add one
using the constants table above (it is a Bitcoin-like X11 coin, same as Dash).

Expose `TCP_PORT`/`SSL_PORT` and publish the host as e.g.
`electrum.dimecoinnetwork.com:10061` (and `:20061` SSL). Then set
`electrum_servers` in the JSON files to that host and submit upstream.

## 2. Submit upstream

- **AtomicDEX**: open a PR to `KomodoPlatform/coins` adding `electrums/DIME`
  (server list) and to the MM2/atomicDEX-API coin list with the constants from
  `dimecoin-atomicdex.json`.
- **BasicSwap**: open a PR to `basicswap/basicswap` adding `coins/dimecoin.json`
  from `dimecoin-basicswap.json`.
- **DCRDEX** / **Block DX** / **THORChain-Maya**: require a running backend /
  chain client (see issue #99). The same constants apply.

## Why this is safe

- No code change to Dimecoin core; only config + an ElectrumX server the team
  already knows how to run.
- No KYC, no custody, no listing fee — consistent with the project's
  decentralization goals and with applicable promotion rules.
