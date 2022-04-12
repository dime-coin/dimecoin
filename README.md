Dimecoin Core integration/staging tree
=====================================

https://dimecoinnetwork.com

Welcome to the Dimecoin repository.  

What is Dimecoin?
----------------

Established in December of 2013, Dimecoin is a decentralized, community focused, and self-funded project. 

Dimecoin is an experimental digital currency that enables instant payments to
anyone, anywhere in the world. Dimecoin uses peer-to-peer technology to operate
with no central authority: managing transactions and issuing money are carried
out collectively by the network. Dimecoin Core is the name of open source
software which enables the use of this currency.

For more information, as well as an immediately useable, binary version of
the Dimecoin Core software, see https://dimecoinnetwork.com/.

Dimecoin Specifications:
-------

| Blockchain Details    |                   |
------------------------|--------------------
Creation Date           | December 23rd, 2013
Proof Type              | Hybrid Proof of Stake (PoS) / Proof of Work (PoW) with Masternodes 
Algo                    | Quark
Block Time              | 64 seconds
Block Reward / Fees     | 15,400 DIME awarded per block. 45% to miners/stakers 45% to masternodes 10% to foundation for development. Fee: 0.0001 DIME / mb
Difficulty              | Adjusted per block
Staking Requirement     | 100,000 DIME
Staking Maturity        | 450 Confirmations
Masternode Collateral   | 500,000,000 DIME
Circulation             | [View on explorer](https://chainz.cryptoid.info/dime/)
Max Supply              | No maximum supply, block reward set to reduce 8% annually. 


License
-------

Dimecoin Core is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Releases](https://github.com/dime-coin/dimecoin/releases) are created
regularly to indicate new official, stable release versions of Dimecoin Core.

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The Travis CI system makes sure that every pull request is built for Windows, Linux, and macOS, and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------
We only accept translation fixes that are submitted through [Bitcoin Core's Transifex](https://www.transifex.com/projects/p/bitcoin/) page. Translations are converted to Dimecoin periodically.

Translations are periodically pulled from Transifex and merged into the git repository. See the translation process for details on how this works.

Important: We do not accept translation changes as GitHub pull requests because the next pull from Transifex would automatically overwrite them again.
