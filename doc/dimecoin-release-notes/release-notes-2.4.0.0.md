### Dimecoin Core version 2.4.0.0
---------------------------------

Dimecoin Core version 2.4.0.0 is available from:

https://github.com/dime-coin/dimecoin/releases/tag/2.4.0.0

This release focuses on reliability, wallet correctness, synchronization, build portability, and defensive hardening.
It is fully compatible with the existing Dimecoin network and does not introduce a hard fork or coordinated activation.

Please report bugs using the GitHub issue tracker:

https://github.com/dime-coin/dimecoin/issues

### Verify Downloads
--------------------

Always verify the SHA-256 hash of a downloaded archive before installing it. The published hashes confirm that the
files match the official release artifacts and have not been corrupted or replaced.

### How to Upgrade
------------------

**Back up every wallet before upgrading, preferably to an external device.**

Wallet backup instructions are available at:

https://www.dimecoinnetwork.com/docs/backup-desktop-wallet/

1. Shut down Dimecoin Core and wait for it to exit completely.
2. Install the new Windows release or replace the existing `dimecoind`, `dimecoin-cli`, `dimecoin-tx`, and
   `dimecoin-qt` binaries on Linux.
3. Start Dimecoin Core normally.

No blockchain resynchronization, wallet conversion, or configuration change is expected when upgrading from
2.3.0.0. Existing wallets remain supported.

If the client was not shut down cleanly or synchronization problems occur, try a reindex after confirming that the
wallet is safely backed up:

https://www.dimecoinnetwork.com/docs/rescan-reindex/

### Compatibility
-----------------

Dimecoin Core 2.4.0.0 remains network-compatible with 2.3.0.0. This release does not change:

- Block or transaction consensus rules
- Dimecoin issuance or staking reward calculations
- Difficulty adjustment or activation heights
- Network protocol versions or serialized blockchain data
- Wallet keys or wallet database format

Operators are encouraged to upgrade promptly to receive the complete set of reliability and hardening improvements.

### Notable Changes
-------------------

#### Network and synchronization

- Improved initial blockchain download performance and peer handling.
- Improved recovery from stalled header and block synchronization.
- Strengthened checkpoint, reorganization, and chain-state handling.
- Reduced unnecessary peer churn and improved synchronization diagnostics.

#### Wallet

- Corrected available and immature balance reporting for staking rewards.
- Applied consistent maturity reporting to spendable, watch-only, and address-grouping balances.
- Improved wallet database, recovery, import, export, and error-handling paths.
- Improved transaction accounting, fee handling, coin selection, and RPC consistency.

#### Masternodes, governance, and services

- Improved masternode synchronization, message handling, and payment diagnostics.
- Improved governance and InstantSend state consistency.
- Improved signed-message compatibility across supported Dimecoin versions.
- Reduced misleading log errors without changing block-payment enforcement rules.

#### Reliability and hardening

- Added stricter validation and safer failure handling across network, RPC, wallet, and service components.
- Added bounds to several previously unrestricted in-memory structures and processing paths.
- Corrected multiple race, locking, cache, and lifecycle issues.
- Improved handling of malformed or inconsistent input.

Some defensive changes are intentionally summarized at a high level to avoid exposing unnecessary implementation
detail before network adoption is widespread.

#### Build system and portability

- Updated the reproducible dependency build for modern Linux systems.
- Added support for current Boost releases, including Boost 1.88.
- Updated bundled dependencies used by portable Linux builds.
- Improved Ubuntu 24.04 and modern compiler compatibility.
- Corrected Qt and MinGW compatibility for Windows cross-compilation.
- Expanded native Linux and Windows build documentation.

#### Testing and quality assurance

- Added the project's dedicated Dimecoin functional QA suite.
- Expanded unit coverage for wallet maturity, message signing, networking, masternodes, validation, and resource bounds.
- Completed clean Linux, dependency-based Linux, and Windows x86-64 builds.
- Completed full testnet and mainnet synchronization from genesis with full historical verification enabled.
- Confirmed chain-tip agreement with existing production nodes.

### Security
------------

This release includes defensive hardening across several subsystems. Detailed reproduction information is not included
in these public release notes. Users, node operators, and service providers should upgrade rather than continuing to
operate older builds on publicly reachable systems.

RPC access should never be exposed directly to the public Internet. Restrict RPC connections to localhost, a trusted
private network, or a properly secured VPN.

### Credits
-----------

Thanks to the Dimecoin contributors, testers, node operators, and community members who helped review and validate this
release.

Direct code contributions for this release include work by:

- Douglas `Dhop14` Hopping
- Sean `Dalamar` Cusack
