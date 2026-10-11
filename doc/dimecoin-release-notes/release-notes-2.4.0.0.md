### Dimecoin Core version 2.4.0.0
---------------------------------

Dimecoin Core version 2.4.0.0 is now available from:

https://github.com/dime-coin/dimecoin/releases/tag/2.4.0.0

This release includes wallet correctness fixes, sync improvements, security hardening, updated build dependencies, and expanded testing of Dimecoin's hybrid PoW and PoS operation. **PLEASE read this guide in its entirety BEFORE updating your software!**

Please report bugs using the issue tracker at GitHub:

https://github.com/dime-coin/dimecoin/issues

To receive security and update notifications, please follow our X (Twitter) account:

https://twitter.com/Dimecoin

### Verify
------------------

Always verify the SHA-256 hash of the archive you downloaded against the checksums published with the official release. A matching hash confirms that your download matches the published file. Obtain both the software and its checksums from the official release page, and do not install an archive if its hash does not match.

### How to Upgrade
----------------------

**WARNING:** Before proceeding, back up every wallet you use, preferably to an external device. Keep your backups in a secure location.

You can create a backup from the wallet's **File > Backup Wallet** menu or with the `backupwallet` RPC. If you copy wallet files manually, shut down Dimecoin Core completely before making the copy.

If you are running an older version, shut it down and wait until the application and any background daemon have completely exited before replacing any files.

Install the package provided for your operating system. On Windows, run the installer or replace the files from the portable package. On Linux, replace the applicable `dimecoind`, `dimecoin-cli`, `dimecoin-tx`, and `dimecoin-qt` binaries. Where a macOS package is provided, replace the Dimecoin-Qt application.

Start Dimecoin Core using your existing data directory and wallet configuration, then allow sync to finish.

**NOTE:** Upgrading from 2.3.0.0 does not require a scheduled hard fork, a new activation height, or a routine blockchain resync. No wallet database format conversion is introduced by this release. 

**Reindex caution:** A full `-reindex` is not required for a routine upgrade from 2.3.0.0. This release includes a partial reindex correction, but rebuilding the complete historical PoW/PoS chain without peer assistance remains an unresolved limitation. Do not rely on `-reindex` as a guaranteed recovery procedure. If database recovery is necessary, secure your wallet backups, preserve the existing data directory, and obtain current project guidance before proceeding. See Known Limitations below.

These upgrade instructions apply to an existing 2.3.0.0 installation. Users migrating from substantially older releases should also review the intervening release notes for earlier database and wallet migration requirements.

**Windows Sync Times:**: Real time antivirus scanning may slow initial blockchain synchronization (IBD). If synchronization is unusually slow, a targeted exclusion in Windows Security for the Dimecoin data directory may help. Exclusions reduce antivirus protection, so limit them to the necessary Dimecoin files and folders, verify your downloaded software, and keep antivirus protection enabled elsewhere.

### Downgrade Warning
----------------------

Although 2.4.0.0 does not introduce a wallet database format change relative to 2.3.0.0, downgrading removes the wallet, sync, and security fixes included in this release. Keep a pre-upgrade backup and shut down the client completely before changing versions.

Compatibility with 2.3.0.0 should not be interpreted as compatibility with every earlier wallet release. Observe the wallet compatibility restrictions documented in the respective release notes before attempting a downgrade to an older version.

### Compatibility
-----------------------

Dimecoin Core 2.4.0.0 is intended to remain compatible with the existing 2.3.0.0 mainnet and testnet networks. It does not introduce a coordinated change to their block or transaction consensus rules, reward calculations, difficulty adjustment rules, or activation heights. The network protocol version is unchanged.

Local relay policy, RPC behavior, sync defaults, and regtest controls do have changes, as described below. Service operators and developers should review these changes when updating integrations.

Use a release package appropriate for your operating system and processor architecture. Build improvements cover modern Linux toolchains and Windows cross compilation. Source builds require Boost 1.60 or newer; the `depends` build uses Boost 1.88.0. Use Berkeley DB 4.8 when building wallets that must remain compatible with the project's distributed binaries.

### Development Notes for 2.4.0.0
-----------------------

**Notable Changes**

This release improves initial blockchain sync, peer handling, and recovery from interrupted sync. Startup reliability has also been improved. The reindex changes address only part of the outstanding reconstruction issues; complete isolated rebuilding of historical PoW/PoS chain data remains unresolved and will be fixed in an upcoming release.

Wallet changes improve available and immature balance reporting for staking outputs, including watch only and address grouping balances. Individual staking transactions continue to report their reward while their outputs mature. Staking input selection and minting estimates have also been improved without changing mainnet or testnet staking reward rules.

Additional work improves wallet loading, database reliability, transaction accounting, and error reporting. Security related improvements are summarized in the Security section below.

Masternode sync, governance, and related services receive reliability and compatibility improvements. The GUI includes corrected masternode controls, more consistent status displays, and other usability fixes.

Developers can now exercise PoW, PoS, and mixed block sequences on regtest through the normal block assembly, coinstake, signing, validation, and processing paths. New tests cover hybrid chain generation, reorgs, and related wallet behavior.

**Changes for Node Operators and Integrations**

- The optional `-maxreorgdepth=<n>` safeguard defaults to **0**, meaning no additional reorg depth limit, on all networks. A positive value explicitly enables a local limit after initial sync and can cause the node to decline a valid higher work chain accepted by nodes using the default. Retain the default unless you deliberately require that policy.
- Mainnet and testnet minimum chainwork and assume valid defaults have been updated using historical reference blocks. Operators who require full historical script verification can use `-assumevalid=0`.
- The default `OP_RETURN` relay/mining policy limit changes from **153 to 83 script bytes**. Applications creating data carrier transactions should check their output sizes and any explicit `-datacarriersize` setting. This is a local transaction policy change, not a new block-consensus limit.
- `walletpassphrase` now accepts its optional `stakingonly` argument through the RPC interface and CLI conversion table. `sendtoaddress` argument mappings are corrected to account for `amount_of_splits` before the fee subtraction and replacement options.
- `setstakesplitthreshold` uses the named argument `value`, accepts values from 0 through 999999, and returns `saved` as a JSON boolean. It reports a database error if the setting cannot be persisted. Integrations using the former `threshold_amount` argument name or string valued `saved` result should be updated.
- A serialization calculation in `gettxoutsetinfo` is corrected. Its `hash_serialized_2` result can therefore differ from 2.3.0.0 for the same UTXO set; this correction does not itself change the UTXO database or coin supply.
- `generatepos` is available **only on regtest**. Regtest now supports on-demand hybrid testing and uses P2P port **31931** by default. These test controls do not enable on demand PoS generation on mainnet or testnet.

### 2.4.0.0 Change Log
-----------------------

**Processing and Sync**

- Improve initial blockchain download performance, peer handling, and sync recovery.
- Improve startup reliability and address a reindex startup issue. Complete isolated reindexing of historical PoW/PoS chain data remains unresolved; see Known Limitations.
- Keep the optional maximum reorg depth disabled by default for mixed-version compatibility.
- Improve mempool reporting and correct the serialized UTXO statistics hash calculation described above.
- Regenerate mainnet fixed seeds from Dimecoin peers, correct seed generation ports, and remove stale Bitcoin testnet seed entries. [#86](https://github.com/dime-coin/dimecoin/pull/86)

**Wallet and Staking**

- Improve staking balance accounting across spendable, watch only, and address grouping views while preserving staking transaction reward reporting.
- Improve wallet loading, database reliability, and transaction state consistency.
- Improve staking input selection, coin age calculations, and minting probability reporting.
- Improve wallet import/export handling, coin selection, fee handling, and error reporting.

**RPC, GUI, and Services**

- Correct DIME money formatting to use five decimal places and improve large amount parsing.
- Correct CLI and named argument handling for wallet, staking, PSBT, and governance commands; improve RPC response consistency and error reporting.
- Correct Dimecoin names, address errors, and port examples in RPC help. [#92](https://github.com/dime-coin/dimecoin/pull/92)
- Fix the GUI's Start Missing masternodes action and improve balance, minting, and masternode count displays.
- Improve GUI export handling and correct the micro DIME unit label.
- Improve REST response consistency and Tor and ZeroMQ service reliability.

**Masternodes and Governance**

- Improve masternode sync, signed message compatibility, and service reliability.
- Improve masternode status reporting and mining template payee information.
- Improve governance and related service consistency.
- Correct misleading masternode sync log names. [7799a19](https://github.com/dime-coin/dimecoin/commit/7799a192e67afcea90716f7f3db5619f2437a8ab)

**Build System**

- Update the `depends` versions of Boost to 1.88.0, libevent to 2.1.12, libxcb to 1.15, and xcb-proto to 1.15.2; improve compatibility with modern compilers and Linux build environments.
- Update the bundled OpenSSL build from 1.0.1k to 1.0.2u and correct dependency generation while retaining disabled algorithm build options. [#89](https://github.com/dime-coin/dimecoin/pull/89)
- Add Berkeley DB build fixes and Qt/MinGW compatibility patches; expand Linux and Windows build instructions.
- Update version information to 2.4.0.0 and refresh the copyright year. [2e17f79](https://github.com/dime-coin/dimecoin/commit/2e17f79e455198ebd0d977378af7c536bc464914), [05e59d1](https://github.com/dime-coin/dimecoin/commit/05e59d1fd380ca0ea542f1a9498ad7905dfccdbf)

**Testing and Documentation**

- Add the Dimecoin functional QA suite with isolated test data and HTML reports.
- Add regtest only `generatepos`, deterministic PoS searches, PoW/PoS and mixed chain generation tests, reorg coverage, and hybrid functional tests in CI. [#144](https://github.com/dime-coin/dimecoin/pull/144)
- Expand unit and regression coverage for wallet operation, networking, and node services.
- Enable GitHub Actions checks for pull requests, update action versions, and improve failure log collection. [#95](https://github.com/dime-coin/dimecoin/pull/95), [#144](https://github.com/dime-coin/dimecoin/pull/144)
- Refresh QA usage, contribution guidance, CI and translation documentation, and the development roadmap. [#137](https://github.com/dime-coin/dimecoin/pull/137), [#86](https://github.com/dime-coin/dimecoin/pull/86), [#138](https://github.com/dime-coin/dimecoin/pull/138), [d86ae99](https://github.com/dime-coin/dimecoin/commit/d86ae991a100417ebff4fb8f4591fc562c5977b4)

### Known Limitations
------------------------

**Full reindexing of historical PoW/PoS chain data:** A full `-reindex` performed without peer assistance can fail to reconstruct the complete historical chain. Version 2.4.0.0 includes a partial correction, but it does not fully resolve a documented issue with reindexing. The included isolated reindex regression test covers a PoW only history and does not establish successful reconstruction of the complete mixed PoW/PoS history. This will be fixed in a future update.

Ordinary peer assisted sync and isolated rebuilding from existing block files are different operations. Successful sync with peers does not demonstrate that an isolated full reindex will succeed. No routine reindex is required when upgrading a working 2.3.0.0 installation.

### Security
------------------------

This release includes security and reliability improvements across wallet, networking, and node services. **Users, node operators, and service providers are strongly encouraged to upgrade.**

Security related changes are intentionally summarized at a high level while users and services update. These public notes omit detailed failure conditions, reproduction instructions, and direct references to individual security fixes.

Keep RPC access restricted to localhost, a trusted private network, or a properly secured VPN. Do not expose the RPC interface directly to the public Internet.

### Credits
------------------------

Thanks to everyone who directly contributed to this release (in no particular order):

- Douglas _Dhop14_ Hopping — [@Dhop14](https://github.com/Dhop14)
- Sean _Dalamar_ Cusack — [@prettyflyforabeeguy](https://github.com/prettyflyforabeeguy)
- [@crytobot459](https://github.com/crytobot459)
- Dango — [@dango](https://github.com/dango)

Thanks also to earlier Dimecoin and upstream contributors, the testers, reviewers, node operators, and community members whose work and feedback continue to support Dimecoin Core.
