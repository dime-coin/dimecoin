Dimecoin Core version 2.1.0.0 is now available from:

  <https://github.com/dime-coin/dimecoin/releases/tag/2.0.0.0>

This is a new minor version release which includes various bugfixes and performance improvements.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dime-coin/dimecoin/issues>

To receive security and update notifications, please follow our Twitter:

  <https://twitter.com/Dimecoin>

How to Upgrade
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

Downgrade Warning
-------------------

Wallets created in 2.1.0.0 and later will not be compatible with versions prior to 2.0.0.0 and will not work if you try to use newly created wallets in older versions. Existing wallets that were created with older versions are not affected by this. 

Compatibility
==============

Dimecoin Core is extensively tested on multiple operating systems using the Linux kernel, macOS 10.8+, Raspberry Pi OS (64bit only), 
and Windows Vista and later. Windows XP is not supported.

Dimecoin Core should also work on most other Unix-like systems but is not frequently tested on them.

2.1.0.0 Change Log
------------------

### Chain and Consensus
- switch to dual-difficulty retargeting algorithm `aaee936` `b2f8c86`

- correct maxfee to appropriately reflect scale of dimecoin `d3f0c6f`

- allow for mnsync from prior proto version when upgrading `bdea657`

- remove peer disconnect after full-sync `08bc486`

### Miscellaneous

- clarification for LogPrint regarding peer disconnect during sync `b46c1f8`
- added a utility for input consolidation`369b842`

Credits
=======

Thanks to everyone who directly contributed to this release (in no particular order):

- James `barrystyle` Taylor
- Douglas `Dhop14` Hopping
- Sean `Dalamar` Cusack
