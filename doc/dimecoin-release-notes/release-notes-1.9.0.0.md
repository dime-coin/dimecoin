Upgrade notes:

* Backup your Dimecoin data directory before upgrading (in particular your wallet.dat)
* A database reindex is required the first time Dimecoin 1.9.0.0 runs on an old installation (=< 1.8.0)
    run qt with: dimecoin-qt -reindex
    run daemon with: dimecoind -reindex --daemon
    or click OK on the prompt to rebuild database.
    Note: This can take a moment.
    
* If you upgrade from 1.9.0-beta, no reindex will happen as database is already at right format. Just backup your wallet.dat.

The code source core was upgraded, which leads to changes that can easily be noticed :

* Internal seeds list updated : more peers connecting 
* much faster blockchain syncing
* connection compatibility with all running versions
