Dimecoin Core version 2.0.0.0 is now available from:

  <https://github.com/dime-coin/dimecoin/releases/tag/2.0.0.0>

This is a new major version release including new features, various bugfixes
and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dime-coin/dimecoin/issues>

To receive security and update notifications, please follow our Twitter:

  <https://twitter.com/Dimecoin>

How to Upgrade
==============

**WARNING**: Before proceeding, make sure to backup your existing wallet -- preferrably on an external device!

Follow the link below for a detailed guide on how to backup your wallet: 

  <https://www.dimecoinnetwork.com/docs/backup-desktop-wallet/>

If you are running an older version, shut it down. Please wait until it completely shuts down and the dialog box closes (this might take a few minutes for older versions) before making any changes. 

Next, run the installer (on Windows) or just copy over `/Applications/Dimecoin-Qt` (on Mac)
or `dimecoind`/`dimecoin-qt` (on Linux).

The first time you run version v2.0.0.0, your chainstate database will be converted to a new format, which will take anywhere from a few minutes to half an hour, depending on the speed of your machine. 
Note that the block database format also changed in version 1.8.0 and there is no automatic upgrade code from before version 1.8 to version 1.10.0 or higher. Upgrading directly from 1.10.x and earlier without re-downloading the blockchain is not supported. This means you will need to fully resync the block database. The best practice is to let it do its thing on the initial startup. 

However, as usual, old wallet versions are still supported. If you do not specify a location for your wallet.dat with the -datadir= flag on startup, you may need to manually move your wallet.dat to the dedicated wallets subfolder within the main Dimecoin directory for your balance to properly reflect.

Default location on Windows:

c:\users\yourusername\appdata\roaming\dimecoin\wallets

Downgrading warning
-------------------

Wallets created in 2.0.0.0 and later will not be compatible with versions prior to 2.0.0.0.0
and will not work if you try to use newly created wallets in older versions. Existing
wallets that were created with older versions are not affected by this.

Compatibility
==============

Dimecoin Core is extensively tested on multiple operating systems using the Linux kernel, macOS 10.8+, Raspberry Pi OS (64bit only), 
and Windows Vista and later. Windows XP is not supported.

Dimecoin Core should also work on most other Unix-like systems but is not frequently tested on them.

2.0.0.0 change log
------------------

### Chain and Consensus
- implement hybrid POW/POS consensus and masternodes, rebase over bitcoin .17 `7fed4b9`
- implement foundation payments `3299258`
- prevent/punish duplicate masternodes `22f6e63`
- sync optimization `cfdd0ac` `1731a78`
- include Secp256k1 `6805c5e`

### RPC and other APIs
- add listminting RPC functionality `de59966`

### GUI
- implement minting tab `367cce9`

### Build system
- update source for qt 5.9 depends packages `d470856`
- update build documentation `07da54b`

### Tests and QA
- finalized unit tests for v2.0.0.0 `5914a85`

### Miscellaneous
- cleanup abandoned stakes `f0e0c16`

Credits
=======

Thanks to everyone who directly contributed to this release (in no particular order):

- James `barrystyle` Taylor
- Douglas `Dhop14` Hopping
- Alexander `CryptoAxl` de Villechenous
- Sean `Dalamar` Cusack
- Sibewu `Sibby` Viwe Yose

Also, many thanks to those who helped with the extensive testing (too many to list!!!)