# I2P SAM client (foundation)

`src/i2p_sam.h` is a self-contained, header-only C++ client for the
[I2P SAM v3 protocol](https://geti2p.net/en/docs/api/samv3). It implements the
control-channel handshake (`HELLO`), `SESSION CREATE`, and `STREAM CONNECT` /
`STREAM ACCEPT`, returning the data socket to the caller. It has no dependency
on the rest of the codebase (only POSIX sockets + the C++ standard library), so
it can be unit-tested without building the full node.

## Why this exists

Dimecoin currently supports Tor hidden services (`src/torcontrol.cpp`) but has
no native I2P transport. This module is the first building block for adding
I2P support: it speaks the SAM protocol that an I2P router (i2pd / Java I2P)
exposes on `127.0.0.1:7656`. Full transport integration (a `NET_I2P` network,
`-i2psam` config, inbound SAM acceptor) is tracked separately and depends on
this client. See the modernization roadmap discussion for the plan.

## Testing

`test_i2p_sam.cpp` exercises the client end-to-end against a mock SAM server.

```sh
make -C contrib/i2p test      # builds and runs against mock_sam.py
# or manually:
python3 contrib/i2p/mock_sam.py &   # listens on 127.0.0.1:7656
g++ -std=c++11 -I src -o /tmp/i2p_test contrib/i2p/test_i2p_sam.cpp
/tmp/i2p_test
```

The mock server only validates protocol framing; it does not perform real I2P
routing. Point the client at a running I2P router's SAM port for a live test.
