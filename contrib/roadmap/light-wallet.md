# Phase 2.1 — Electrum-DIME light wallet (BIP158 / Neutrino)

Dimecoin already has the building blocks for a light wallet:

- **BIP158 block filters** implemented (PR #97) + **filter index** (PR #98).
- **ElectrumX server** config + runbook in `contrib/dex/electrumx/`.

This spec lets the team stand up a mobile/desktop wallet that needs no full node.

## Option A — Electrum-style wallet (recommended, fastest)

Fork [spesmilo/electrum](https://github.com/spesmilo/electrum) (or the Electron Cash line) and add a Dimecoin coin:

```python
# electrum/dimecoin/__init__.py (coin params)
NET = "mainnet"
BIP44_COIN_TYPE = 0   # TODO: assigned SLIP44 (see hardware-wallet.md)
WIF_PREFIX = 0x8F
ADDRTYPE_P2PKH = 15
ADDRTYPE_P2SH = 9
BECH32_HRP = "vx"      # after SegWit (Phase 1.1)
HEADERS_URL = None     # use ElectrumX in contrib/dex/electrumx/
```

Point the wallet at `electrumx1.dimecoinnetwork.com:50002` (SSL) / `:50004` (ws) from
`electrums/DIME` (after the team rebuilds them — see issue #99).

## Option B — Neutrino (BIP158) mobile

Use a BIP158 client (e.g. `neutrino` / `bwt`) pointed at a Dimecoin full node with the
filter index enabled (`-blockfilterindex=1`, from PR #97/#98). No ElectrumX dependency.

## Prerequisites the team must satisfy first

1. **ElectrumX rebuilt** with correct genesis (`00000d5a…`) — issue #99 (operational).
2. **SegWit** (Phase 1.1) for `vx` native addresses; until then wallet uses legacy P2PKH.
3. **Hardware wallet** params (hardware-wallet.md) if signing via Trezor/Ledger.

## Deliverables for the team

- [ ] Fork Electrum, add `dimecoin` coin module (params above).
- [ ] Wire ElectrumX hosts from `electrums/DIME`.
- [ ] Release signed binaries; announce.
- [ ] (Optional) Neutrino mobile using BIP158 index.

This is a separate repo from `dime-coin/dimecoin`; only the params above come from this repo.
