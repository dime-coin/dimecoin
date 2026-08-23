# Dimecoin on THORChain / Maya — team-ready integration blueprint

> Scope note: this is **external** to the Dimecoin repo. DIME already has every
> on-chain protocol these DEXs need (JSON-RPC daemon, live ElectrumX, CLTV/CSV
> for HTLC swaps, P2PKH, PSBT). THORChain/Maya already support non-segwit UTXO
> chains (DOGE, BCH, and DASH on Maya), so **SegWit is NOT required**. What is
> missing is the external chain-client + node + governance work below. This
> document is prepared so the team only has to review/approve and then run the
> node + vote — no new Dimecoin-core code is needed.

## Why this is feasible

- DIME is a Bitcoin-style UTXO chain (Quark, **not** X11). THORChain's `utxo/`
  shared client already handles BTC, LTC, BCH, DOGE and **DASH**. DIME slots in
  the same way, but note Dimecoin's transaction version 2 carries a
  Dimecoin-specific `strTxComment` field, so a DASH client clone needs Dimecoin's
  own transaction handling.
- The DASH client (`bifrost/pkg/chainclients/dash/client.go`) is the direct
  template — copy it and change the constants.

## Where the code lives (THORNode / Maya)

Chain clients are in the **THORNode** repo (GitLab, `develop` branch):
`gitlab.com/thorchain/thornode`, under `bifrost/pkg/chainclients/`.
Maya is a THORChain fork: `gitlab.com/mayachain/mayanode` (Maya already has
DASH, so it is the faster path). Start from Maya.

## Step-by-step (team executes)

1. **Evaluation / proposal.** Fill the Chain Proposal Template (see bottom) and
   post to the THORChain/Maya community for Node-Mimir approval. No code yet.
2. **Fork** `mayachain/mayanode` (or `thorchain/thornode`) on `develop`.
3. **Create the client** `bifrost/pkg/chainclients/dimecoin/` by copying
   `dash/` and adjusting `client.go` + the chain config:
   - ticker `DIME`, chain name `DIME`.
   - drop in the DIME params from the sheet below.
   - register it in `bifrost/pkg/chainclients/loadchains.go`.
4. **thornode common changes** (model on how DASH/BTC were added):
   - `common/chain.go` — add `DIME` chain var + `GetGasAsset`.
   - `common/asset.go` — define the `DIME` asset.
   - `common/address.go` — parse DIME addresses (base58 prefix 15/9, bech32
     HRP `vx`).
   - `common/pubkey.go` — `GetAddress(chain)` for DIME.
   - `common/gas.go` — gas-price update logic.
5. **Docker components** — add `build/docker/components/newchain.yml` (and the
   linux variants) so the DIME client runs in mocknet/regtest for the smoke
   test.
6. **xchainjs** — add a DIME entry in the `xchainjs` repo (address/util) so
   wallets/UI can parse DIME.
7. **Audit** — an independent reviewer (not the author) must review the client;
   publish the audit in the PR under `bifrost/pkg/chainclients/dimecoin/`.
8. **Governance** — once merged, a Node-Mimir vote activates DIME on the network.
9. **Run infra** (team): a THORChain/Maya node + a **DIME Bifrost observer**
   that watches a DIME full node / ElectrumX (`electrumx1/2.dimecoinnetwork.com`
   already exist). Provide a DIME node + ElectrumX the observer can reach.
10. **Seed liquidity** for DIME pools.

## DIME parameter sheet (paste into the client + common)

```
ticker:            DIME
chain name:        DIME
pubkey_address:    15   (0x0F)
script_address:    9    (0x09)
wif_prefix:        143  (0x8F)
bech32 HRP:        vx
native segwit:     no
algorithm:         Quark
RPC port:          8332
P2P port:          11931   (testnet 21931, regtest 31931)
tx version:        2
genesis hash:      00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da
message start:     fea503dd
ElectrumX (live):  electrumx1.dimecoinnetwork.com:50001 (SSL :50002 / WS :50004)
                   electrumx2.dimecoinnetwork.com:50001
seed nodes:        seed1/seed2.dimecoinnetwork.com, node1/node2.dimecoinnetwork.com
```

## Filled Chain Proposal Template (team fills market data)

```
Chain Name:        Dimecoin (DIME)
Chain Type:        UTXO
Consensus:         hybrid PoW (Quark) / PoS
Hardware Req:      (fill: ~2 vCPU / 4GB for a full node + observer)
Year Started:      2014 (verify)
Market Cap:        (team fills)
CoinMarketCap Rank:(team fills)
24h Volume:        (team fills)
Current DEX:       AtomicDEX (live), BasicSwap (PR tecnovert/basicswap#3)
Other dApps:       (team fills)
Prev hard forks:   (team fills)
```

## What the team must own (cannot be done here)

- Running the THORChain/Maya node + DIME observer (infra).
- The Node-Mimir governance vote.
- Liquidity seeding.
- The actual Go implementation (large monorepo; scaffolded above as exact files
  - params, to be completed by a dev in the thornode/mayanode repo).

## References

- THORChain new-chain guide: https://dev.thorchain.org/new-chains/implementation-guide.html
- Chain-client layout: `bifrost/pkg/chainclients` (THORNode `develop`)
- DASH client template: `bifrost/pkg/chainclients/dash/client.go`
- DEX status (this repo): issue #99, PR #100; BasicSwap: tecnovert/basicswap#3
