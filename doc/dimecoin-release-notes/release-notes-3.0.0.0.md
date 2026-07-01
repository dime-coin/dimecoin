### Dimecoin Core version 3.0.0.0
---------------------------------
Dimecoin Core version 3.0.0.0 is now available from:

https://github.com/dime-coin/dimecoin/releases/tag/3.0.0.0

This is a **major** version release which includes consensus-affecting changes, bugfixes, and improvements. **PLEASE read this guide in its entirety BEFORE updating your software!**

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

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then run the installer (on Windows) or just copy over `/Applications/Dimecoin-Qt` (on macOS) or `dimecoind`/`dimecoin-qt` (on Linux).

**IMPORTANT:** This is a major release that contains consensus-affecting changes. All node operators, miners, stakers, and exchanges **MUST** upgrade prior to the activation height to remain on the main chain.

### Compatibility
------------------
Dimecoin Core is supported and extensively tested on operating systems using the Linux kernel, macOS 10.12+, and Windows 7 and newer. It is not recommended to use Dimecoin Core on unsupported systems.

### Notable Changes
---------------------

This release incorporates the consensus-risk remediation set landed on the `dev_plus_consensus_risks` branch, along with the audit and defensive bug fixes shipped in prior 2.x releases. Refer to the git log between the `2.3.0.0` and `3.0.0.0` tags for the full list of changes.

### Credits
------------
Thanks to everyone who directly contributed to this release, as well as everyone who submitted issues, reviewed pull requests and helped debug the reported issues.
