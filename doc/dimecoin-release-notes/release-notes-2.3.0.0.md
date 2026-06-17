### Dimecoin Core version 2.3.0.0
---------------------------------
Dimecoin Core version 2.3.0.0 is now available from:

https://github.com/dime-coin/dimecoin/releases/tag/2.3.0.0

This is a minor version revision which includes various bugfixes and performance improvements. **PLEASE read this guide in its entirety BEFORE updating your software!**

Please report bugs using the issue tracker at GitHub:

https://github.com/dime-coin/dimecoin/issues

To receive security and update notifications, please follow our Twitter:

https://twitter.com/Dimecoin

### Verify
------------------
Verifying the hashes of the archive you downloaded is strongly advised, and will confirm that the files you downloaded match those uploaded by Dimecoin's contributors. While it may seem trivial to some, checking the hashes of your downloads should be considered an essential step in updating your clients. A corrupted download may result in lost funds or a compromised wallet. You should always verify your downloads! A list of the corresponding hashes has been appended below in the sha256sums.txt file.

### How to Upgrade
----------------------

**WARNING:** Before proceeding, make sure to backup your existing wallet -- preferably on an external device!

Follow the link below for a detailed guide on how to backup your wallet:

https://www.dimecoinnetwork.com/docs/backup-desktop-wallet/

**NOTE:** Protocol changes will go into effect at block _5500000_. _**PLEASE**_ update your wallets/nodes _**PRIOR**_ to the block height being reached to ensure proper functionality.

If you are running an older version, shut it down. Please wait until it completely shuts down and the dialog box closes (this might take a few minutes for older versions) before making any changes.

Next, run the installer (on Windows) or just copy over /Applications/Dimecoin-Qt (on Mac) or dimecoind/dimecoin-qt (on Linux).

If this is the first time you are running a version post 1.10.0.1, your chainstate database will be converted to a new format, which will take anywhere from a few minutes to half an hour or longer, depending on the speed of your machine.

Note that the block database format also changed in version 1.8.0 and there is no automatic upgrade code from before version 1.8 to version 1.10.0 or higher. Upgrading directly from 1.10.x and earlier without re-downloading the blockchain is not supported. This means you will need to fully resync the block database. The best practice is to let it do its thing on the initial startup.

However, as usual, old wallet versions are still supported. If you do not specify a location for your wallet.dat with the -datadir= flag on startup, you may need to manually move your wallet.dat to the dedicated wallets subfolder within the main Dimecoin directory for your balance to properly reflect.

_Default location on Windows:_ c:\users\your-user-name\appdata\roaming\dimecoin\wallets

Existing users who experience issues syncing, may have unintentionally corrupted their blockchain database files during the upgrade process. If you run into these issues, reindexing will fix the problem most of the time! [Here](https://www.dimecoinnetwork.com/docs/rescan-reindex/) is a guide that explains how to perform a reindex. If the issue persists, it is recommended to backup your wallet.dat and start with a fresh data directory. 

As always, be sure to shut down your client if it's running, and backup your wallets before performing any updates or maintenance actions!!  

### Downgrade Warning
----------------------
Wallets created in 2.3.0.0 and later will not be compatible with versions prior to 2.0.0.0 and will not work if you try to use newly created wallets in older versions. Legacy wallets that were created with older versions (prior to 2.0.0.0) are not affected and will work with v2.3.0.0. 

### Compatibility
-----------------------
Dimecoin Core is extensively tested on multiple operating systems using the Linux kernel, macOS 10.8+, Raspberry Pi OS (64bit only), and Windows Vista and later. Windows XP is not supported.

Dimecoin Core should also work on most other Unix-like systems but is not frequently tested on them.

### Development Notes for 2.3.0.0
-----------------------

**Notable Changes**

Changes have been made to Dimecoin Core that deal with sync optimization, input selection during the staking process, staking has been further optimization based on proof hashes, and additional protection added which prevents inputs with bad signatures from staking. 

Locked collaterals, and coin control have been refactored to allow locked coins to exist without selectCoins returning false, which would previously halt staking. Additional locks have been placed to prevent manually locked inputs from being spent from coin control.  

QT has been patched for limits that fixed compiling the QT package in depends with GCC 11+.

### 2.3.0.0 Change Log
-----------------------

**Build System**

- Patch QT for limits to fix compiling the QT package in depends with GCC 11+[4285b9](https://github.com/dime-coin/dimecoin/commit/4285b9b485e717e875a1ffdddafc62bd4032e991).

**Processing**

- ensure IBD finishes before staking is enabled ['21963b'](https://github.com/dime-coin/dimecoin/commit/21963b29e159e623830808313fee6c1a3b11c460)

- avoid staking inputs with bad signatures ['3aa3ef'](https://github.com/dime-coin/dimecoin/commit/3aa3effb3946e09d548e39107e144d3e87a241fb)

- resolve indexing issue to avoid assertion error during sync [62b85a](https://github.com/dime-coin/dimecoin/commit/62b85a637ebb6b2a1dbabb2b9355a743c73f2f28)

- ensure staking is active, even when the user manually locks inputs [d5031d](https://github.com/dime-coin/dimecoin/commit/d5031db5ae8f8797a11cc7ba0ea7c79be9aa0550)

**Wallet**

- prevent manually locked coins from being eligible from staking [0ce923](https://github.com/dime-coin/dimecoin/commit/0ce923a4e9241371df9fb280bd89ff7c81f22c9f)

- clean up redundancy of masternode collaterals from the AvailableCoins loop since they are not spendable [79ea7d](https://github.com/dime-coin/dimecoin/commit/79ea7dcc59e9e43984dc9a7af7d0597f94d23b90)

### Credits
------------------------

Thanks to everyone who directly contributed to this release (in no particular order):

- James _barrystyle_ Taylor
- Douglas _Dhop14_ Hopping
- Sean _Dalamar_ Cusack
