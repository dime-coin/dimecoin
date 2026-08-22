# Dimecoin on DCRDEX — team-ready integration blueprint

> Scope note: external to the Dimecoin repo. DIME already has every protocol
> DCRDEX needs (Bitcoin-style JSON-RPC daemon, live ElectrumX, standard UTXO
> tx, CLTV/CSV). DCRDEX supports UTXO clones (BTC, LTC, BCH, DOGE, **DASH**) via
> a shared `btc` backend, so DIME slots in by cloning the DASH asset. No
> Dimecoin-core change is required — the team only reviews/approves and runs
> the DEX client (traders point it at their own `dimecoind` / ElectrumX).

## Where the code lives (decred/dcrdex, `master`)

- Shared UTXO backend: `dex/btc` (`BTCCloneWallet`).
- Per-coin network params: `dex/networks/<coin>/` — copy `dex/networks/dash`.
- Per-coin asset driver: `client/asset/<coin>/<coin>.go` — copy
  `client/asset/dash/dash.go`. It self-registers via `asset.Register(...)`.

## Step-by-step (team executes)

1. **Proposal.** Open a DCRDEX asset proposal (Politeia / DEX-operator request)
   referencing issue decred/dcrdex#3626; get DEX operators to agree to list DIME.
2. **Fork** `decred/dcrdex` on `master`.
3. **Create `dex/networks/dimecoin/`** by copying `dex/networks/dash`, setting:
   - `MainNetParams` / `TestNetParams` / `RegressionNetParams` (pubkeyhash 15,
     scripthash 9, wif 143, hdpub 76067358, hdpriv 76066276, bech32 `vx`,
     RPC 8332, P2P 11931).
   - `UnitInfo` with 5 decimal places.
   - `DefaultFee` / fee-rate limits (model on DASH).
4. **Create `client/asset/dimecoin/dimecoin.go`** by copying
   `client/asset/dash/dash.go` and adjusting:
   - `configOpts = append(btc.RPCConfigOpts("Dimecoin", "8332"), ...)`.
   - `WalletInfo{ Name: "Dimecoin", DisplayName: "Dimecoin",
BlockchainClass: asset.BlockchainClassUTXO, UnitInfo: dexdimecoin.UnitInfo }`.
   - `cloneCFG` uses `dexdimecoin.MainNetParams` (etc.) and the DIME params sheet
     below; `NonSegwitSigner`/segwit flags set to match DIME (no native segwit).
   - `func init() { asset.Register(BipID, &Driver{}) }` — self-registers.
   - `BipID` (the DCRDEX asset ID) — set a value (placeholder `15`; confirm the
     intended asset ID, since DIME is not in SLIP-44).
5. **Wire the import** so the driver loads: add a blank import of
   `client/asset/dimecoin` in the asset loader (`client/asset/asset.go` or the
   package that imports all asset drivers), so its `init()` runs.
6. **ElectrumX (optional).** DIME already has live ElectrumX
   (`electrumx1/2.dimecoinnetwork.com`); DCRDEX's `btc` backend can use ElectrumX
   so traders need not run a full `dimecoind`. Set the Electrum servers in the
   wallet config.
7. **Build + test** (`go test ./client/asset/dimecoin/...`, `go build ./...`).
8. **Open the PR** to `decred/dcrdex`; once merged + operators enable the asset,
   users can trade DIME (pointing the client at their own `dimecoind`/ElectrumX).

## DIME parameter sheet (paste into `dex/networks/dimecoin` + `dimecoin.go`)

```
name:              Dimecoin
ticker:            DIME
asset ID (BipID):  (placeholder 15 — confirm)
pubkeyhash:        15   (0x0F)
scripthash:        9    (0x09)
wif:               143  (0x8F)
hdpub:             76067358  (0x0488B21E)
hdpriv:            76066276  (0x0488ADE4)
bech32 HRP:        vx
native segwit:     no
RPC port:          8332
P2P port:          11931   (testnet 21931, regtest 31931)
tx version:        2
decimal places:    5
genesis hash:      00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da
message start:     fea503dd
ElectrumX (live):  electrumx1.dimecoinnetwork.com:50001 (SSL :50002 / WS :50004)
                   electrumx2.dimecoinnetwork.com:50001
```

## What the team must own (cannot be done here)

- The DCRDEX Politeia/operator proposal + operator enablement.
- Reviewing/merging the `decred/dcrdex` PR (external maintainers).
- Running a `dimecoind` / ElectrumX for their own client (traders provide their
  own; the DEX does not custody).

## References

- DASH template: `client/asset/dash/dash.go`, `dex/networks/dash`
- Shared backend: `dex/btc` (`BTCCloneWallet`)
- DEX status (this repo): issue #99, PR #100; BasicSwap: tecnovert/basicswap#3
- DCRDEX inquiry: https://github.com/decred/dcrdex/issues/3626
