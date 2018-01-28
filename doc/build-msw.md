WINDOWS dimecoin wallet build instructions
===========================================

Compilers Supported
-------------------
Use g++ 4.6 or g++ 4.9

Install QT 4.8
---------------
Install qt-win-opensource-4.8.5-mingw.exe from here http://download.qt-project.org/archive/qt/4.8/4.8.5/


Dependencies
------------
Required libraries have been provided here: https://github.com/halfirish83/builds/tree/master/1.7.0.2/Windows/deps just download and unpack into 

	C:\deps

They are already built on windows, but sourcecode is included so that you can rebuild them if required.

Their licenses:

	OpenSSL        Old BSD license with the problematic advertising requirement
	Berkeley DB    New BSD license with additional requirement that linked software must be free open source
	Boost          MIT-like license
	miniupnpc      New (3-clause) BSD license

Versions used in this release:

	OpenSSL      1.0.1j
	Berkeley DB  5.0.32.NC
	Boost        1.55.0
	miniupnpc    1.9


build OpenSSL
-------
MSYS shell:

un-tar sources with MSYS 'tar xfz' to avoid issue with symlinks (OpenSSL ticket 2377)
change 'MAKE' env. variable from 'C:\MinGW32\bin\mingw32-make.exe' to '/c/MinGW32/bin/mingw32-make.exe'

	cd c:\deps\openssl-1.0.1j
	Configure no-shared no-dso mingw
	make

Build Berkeley DB
-----------
MSYS shell:

	cd c:\deps\db-5.0.32.NC\build_unix
	sh ../dist/configure --enable-mingw --enable-cxx --disable-replication
	make

Build Boost
-----
DOS prompt:

	cd c:\deps\boost_1_55_0
	bootstrap.bat mingw
	b2 --build-type=complete --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc variant=release link=static threading=multi runtime-link=static stage

Build MiniUPnPc
---------
UPnP support is optional, make with `USE_UPNP=` to disable it.

MSYS shell:

	cd C:\deps\miniupnpc
	mingw32-make -f Makefile.mingw init upnpc-static

Build Dimecoin
-------
First build leveldb, in MSYS shell:

	cd /C/dimecoin/src/leveldb
	TARGET_OS=NATIVE_WINDOWS make libleveldb.a libmemenv.a

then from DOS prompt:

	cd dimecoin
	make clean
	qmake
	mingw32-make -f makefile.release
