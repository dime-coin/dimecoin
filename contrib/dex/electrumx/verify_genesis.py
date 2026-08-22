#!/usr/bin/env python3
"""Verify a Dimecoin ElectrumX server serves the correct genesis block.

After rebuilding ElectrumX from a synced dimecoind, run this against each
server to confirm height 0 equals the canonical Dimecoin genesis.

Usage:
    python3 verify_genesis.py electrumx1.dimecoinnetwork.com 50001
    python3 verify_genesis.py electrumx2.dimecoinnetwork.com 50001
"""

import json
import socket
import sys

CANON_GENESIS = "00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da"


def rpc(host, port, method, params, timeout=15):
    s = socket.create_connection((host, port), timeout=timeout)
    s.settimeout(timeout)
    s.sendall(
        (
            json.dumps({"jsonrpc": "2.0", "id": 1, "method": method, "params": params})
            + "\n"
        ).encode()
    )
    buf = b""
    while b"\n" not in buf:
        d = s.recv(4096)
        if not d:
            break
        buf += d
    s.close()
    return json.loads(buf.split(b"\n", 1)[0].decode())


def main():
    if len(sys.argv) < 3:
        print("usage: verify_genesis.py <host> <port>")
        sys.exit(2)
    host, port = sys.argv[1], int(sys.argv[2])
    try:
        ver = rpc(host, port, "server.version", [])
        best = rpc(host, port, "blockchain.headers.subscribe", []).get("result", {})
        h0 = rpc(host, port, "blockchain.block.header", [0]).get("result")
    except Exception as e:
        print(f"{host}:{port} ERROR: {e}")
        sys.exit(1)
    ok = h0 == CANON_GENESIS
    print(f"host            : {host}:{port}")
    print(f"server.version  : {ver.get('result')}")
    print(f"best height     : {best.get('height')}")
    print(f"height 0 hash   : {h0}")
    print(f"canon genesis   : {CANON_GENESIS}")
    print(f"GENESIS OK      : {ok}")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
