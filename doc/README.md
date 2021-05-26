Dimecoin Core
=============

Setup
---------------------
Dimecoin Core is the original Dimecoin client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Dimecoin transactions (which is currently more than 10 GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

To download Dimecoin Core, visit [dimecoinnetwork.com](https://dimecoinnetwork.com/).

Running
---------------------
The following are some helpful notes on how to run Dimecoin Core on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/dimecoin-qt` (GUI) or
- `bin/dimecoind` (headless)

### Windows

Unpack the files into a directory, and then run dimecoin-qt.exe

### macOS

Drag Dimecoin Core to your applications folder, and then run Dimecoin Core.

### Need Help?

* See the documentation at the [Dimecoin Database](https://dimecoinnetwork.com/docs)
for help and more information.
* Ask for help in [Telegram Support](http://t.me/dimeofficialsupport) on Telegram. I
* Ask for help via [Email](mailto:support@dimecoinnetwork.com)

Building
---------------------
The following are developer notes on how to build Dimecoin Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [NetBSD Build Notes](build-netbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Dimecoin repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)]
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [Telegram](t.me/dimecoinofficialpublic)
* Discuss general Dimecoin development in #development on our [Discord](https://discord.gg/JqcKF4v). 

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)
- [PSBT support](psbt.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
