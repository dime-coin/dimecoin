Pre-release beta version for testnet

This pre-release contains the following evolutions currently for testnet only, activated block 30000.

    LWMA-3 difficulty adjustment algorithm
    New reward distribution (no more modulo 1024. Fixed reward instead, decreasing 8% each year).

For using testnet, start the wallet with "-testnet" option after adding the following to dimecoin.conf :

addnode=testnet-node.dimecoinnetwork.com

The wallet will act as usual on mainnet.
