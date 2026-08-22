# Phase 2.2 — Hardware wallet registration (Trezor / Ledger / SLIP44)

Dimecoin is **not** registered with hardware wallets or SLIP44. This package gives the
team a copy-paste submission. All values verified from `src/chainparams.cpp` / `chainparamsbase.cpp`.

## Verified Dimecoin parameters

| Field                 | Value                               | Source                                   |
| --------------------- | ----------------------------------- | ---------------------------------------- |
| P2PKH address version | `15` (`0x0F`)                       | `chainparams.cpp`                        |
| P2SH address version  | `9` (`0x09`)                        | `chainparams.cpp`                        |
| WIF prefix            | `143` (`0x8F`)                      | `chainparams.cpp`                        |
| bech32 HRP            | `vx`                                | `chainparams.cpp` (`segwit:false` today) |
| RPC port              | `8332`                              | `chainparamsbase.cpp:36`                 |
| P2P port              | `11931`                             | `chainparams.cpp`                        |
| Consensus             | PoW Quark, UTXO (Bitcoin Core fork) | `src/hash.h:238`                         |

## SLIP44 coin type

Dimecoin has **no assigned SLIP44 coin type** yet. Request one at
https://github.com/satoshilabs/slips (SLIP-44) — pick a free number and reference this
table. Until assigned, use the temporary value in submissions and note it clearly.

## Trezor submission (`coins` def, `trezor-common`)

```json
{
  "coin_name": "Dimecoin",
  "coin_shortcut": "DIME",
  "address_type": 15,
  "address_type_p2sh": 9,
  "wif_prefix": 143,
  "slip44": 0, // TODO: replace with assigned SLIP44 number
  "bech32_prefix": "vx", // usable after SegWit activation (Phase 1.1)
  "minfee_kb": 100000,
  "maxfee_kb": 1000000000,
  "hash_genesis": "00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da"
}
```

## Ledger submission (`ledger-app-builder` / `crypto-assets`)

- Same `address_type`/`address_type_p2sh`/`wif_prefix` as above.
- Provide a `family` = Bitcoin-like, curve `secp256k1`.
- Link `github.com/dime-coin/dimecoin` as upstream.

## Steps (team runs)

1. File SLIP44 issue with the table above → get `slip44` number.
2. Open Trezor `trezor-common` PR with the JSON (fill `slip44`).
3. Open Ledger `crypto-assets` PR with the same params.
4. After SegWit (Phase 1.1) activates, enable `bech32_prefix` and native segwit paths.
5. Ship firmware support; announce to community.

No code in this repo is required — this is purely external registration using existing constants.
