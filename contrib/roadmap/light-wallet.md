# Phase 2.1 — Electrum-DIME light wallet (BIP158 / Neutrino)

Dimecoin already has purpose-built light-wallet infrastructure — **do not fork
generic Electrum/ElectrumX**:

- [`dime-coin/electrum-dimecoin`](https://github.com/dime-coin/electrum-dimecoin)
  and
  [`dime-coin/electrumx-dimecoin`](https://github.com/dime-coin/electrumx-dimecoin)
  are the Dimecoin-specific Electrum stack.
- **BIP158 block filters** (PR #97) + **BIP157 filter index** (PR #98) provide a
  separate **Neutrino** light-client path that needs no ElectrumX.

This spec lets the team stand up a mobile/desktop wallet that needs no full node.

> Note: Dimecoin's transaction version 2 carries a Dimecoin-specific
> `strTxComment` serialization, so the backend must implement Dimecoin's own tx
> logic (a Dash/Bitcoin clone will not).

## Option A — Electrum-DIME wallet (recommended, fastest)

Use [`dime-coin/electrum-dimecoin`](https://github.com/dime-coin/electrum-dimecoin)
and add/verify the Dimecoin coin params:

```python
# electrum/dimecoin/__init__.py (coin params)
NET = "mainnet"
BIP44_COIN_TYPE = 0   # TODO: assigned SLIP44 (see hardware-wallet.md)
WIF_PREFIX = 0x8F
ADDRTYPE_P2PKH = 15
ADDRTYPE_P2SH = 9
BECH32_HRP = "vx"      # after SegWit (Phase 1.1)
HEADERS_URL = None     # use electrumx-dimecoin
```

Point the wallet at the ElectrumX-Dime host (e.g.
`electrumx1.dimecoinnetwork.com:50002` SSL / `:50004` ws).

## Option B — Neutrino (BIP158) mobile

Use a BIP158 client (e.g. `neutrino` / `bwt`) pointed at a Dimecoin full node with the
filter index enabled (`-blockfilterindex=1`, from PR #97/#98). No ElectrumX dependency.

## Prerequisites the team must satisfy first

1. Run the Dimecoin-specific ElectrumX from `dime-coin/electrumx-dimecoin`.
2. **SegWit** (Phase 1.1) for `vx` native addresses; until then wallet uses legacy P2PKH.
3. **Hardware wallet** params (hardware-wallet.md) if signing via Trezor/Ledger.

## Deliverables for the team

- [ ] Use `dime-coin/electrum-dimecoin`, add/verify `dimecoin` coin module (params above).
- [ ] Wire ElectrumX hosts from `electrums/DIME`.
- [ ] Release signed binaries; announce.
- [ ] (Optional) Neutrino mobile using BIP158 index.

This is a separate repo from `dime-coin/dimecoin`; only the params above come from this repo.
