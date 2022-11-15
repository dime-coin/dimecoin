Dimecoin Core version 2.2.0.0 is now available from:

  <https://github.com/dime-coin/dimecoin/releases/tag/2.2.0.0>

This is a minor version revision which includes various bugfixes and performance improvements.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dime-coin/dimecoin/issues>

To receive security and update notifications, please follow our Twitter:

  <https://twitter.com/Dimecoin>

**How to Upgrade**
==============

**WARNING**: Before proceeding, make sure to backup your existing wallet -- preferably on an external device!

Follow the link below for a detailed guide on how to backup your wallet: 

  <https://www.dimecoinnetwork.com/docs/backup-desktop-wallet/>

If you are running an older version, shut it down. Please wait until it completely shuts down and the dialog box closes (this might take a few minutes for older versions) before making any changes. 

Next, run the installer (on Windows) or just copy over `/Applications/Dimecoin-Qt` (on Mac)
or `dimecoind`/`dimecoin-qt` (on Linux).

If this is the first time you are running a version post 1.10.0.1, your chainstate database will be converted to a new format, which will take anywhere from a few minutes to half an hour, depending on the speed of your machine. 

Note that the block database format also changed in version 1.8.0 and there is no automatic upgrade code from before version 1.8 to version 1.10.0 or higher. Upgrading directly from 1.10.x and earlier without re-downloading the blockchain is not supported. This means you will need to fully resync the block database. The best practice is to let it do its thing on the initial startup. 

However, as usual, old wallet versions are still supported. If you do not specify a location for your wallet.dat with the -datadir= flag on startup, you may need to manually move your wallet.dat to the dedicated wallets subfolder within the main Dimecoin directory for your balance to properly reflect.

Default location on Windows:

c:\users\yourusername\appdata\roaming\dimecoin\wallets

**Downgrade Warning**
-------------------

Wallets created in 2.2.0.0 and later will not be compatible with versions prior to 2.0.0.0 and will not work if you try to use newly created wallets in older versions. Existing wallets that were created with older versions are not affected by this. 

**Compatibility**
==============

Dimecoin Core is extensively tested on multiple operating systems using the Linux kernel, macOS 10.8+, Raspberry Pi OS (64bit only), 
and Windows Vista and later. Windows XP is not supported.

Dimecoin Core should also work on most other Unix-like systems but is not frequently tested on them.

Development Notes for 2.2.0.0
================

**Notable Changes**
---------------------

Changes have been made to Dimecoin Core that deal with peer connectivity and protocol forking. Peers will communicate with the appropriate protocol version to avoid confusion during IBD. 

Changes have also been made to correct the masternode payment split when getblocktemplate is called for Proof of Work. This change will ensure MN pays are split evenly between both Proof of Stake and Proof of Work as intended. 

Additionally, the `getchaintips` RPC function has been explanded to add additional chain fork height parameters for debugging purposes. 

2.2.0.0 Change Log
------------------

### Chain and Consensus

- set more recent chainTx data ['941fee'](https://github.com/dime-coin/dimecoin/commit/941f4ee0f9f7f37728deb03b846ca1d13426f33f)

- reset sporkkey pair ['c9b383'](https://github.com/dime-coin/dimecoin/commit/c9b3835ffeb8e2633161eda363b035e561908d16)

- correct masternode payment split in getblocktemplate for POW  ['5e3168'](https://github.com/dime-coin/dimecoin/commit/5e316859cb84215b65f786ca006ea56d1988a986)

- ensure outputs are correctly signed ['67708b'](https://github.com/dime-coin/dimecoin/commit/67708b444b02b5b2102b842c8c02720e7f76d688)

- remove deprecated fakestake code ['1ec405'](https://github.com/dime-coin/dimecoin/commit/1ec405c9d1a397c7d2813867923de093c0d30d93)

- refactor input tests ['cfc72a'](https://github.com/dime-coin/dimecoin/commit/cfc72a3e01d61f1e91fc8b816abf16f14c2ea88b)

### Processing

- disable protocol forking and bump minprotocol to address forking issues ['969770'](https://github.com/dime-coin/dimecoin/commit/969770616ddf8dc0e14c89c99cc73ddfc8c6f068)

-avoid warning for stakemodifier, display headers while syncing ['2b24c7'](https://github.com/dime-coin/dimecoin/commit/2b24c7c522cadc74745d57f674f995416b7fde54)

### RPC

- expand 'getchaintips' rpc command to assist in fork debugging ['c43df0'](https://github.com/dime-coin/dimecoin/commit/c43df06907575a8ac6b9294ee944eab3f95e9cf8)

Credits
=======

Thanks to everyone who directly contributed to this release (in no particular order):

- James `barrystyle` Taylor
- Douglas `Dhop14` Hopping
- Sean `Dalamar` Cusack
- Alex `AxlCrpto` de Villechenous
