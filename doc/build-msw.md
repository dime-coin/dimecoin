WINDOWS dimecoin wallet build instructions
===========================================

Compilers Supported
-------------------
Use g++ 4.6 or g++ 4.9


Dependencies
------------
Required libraries have been provided here: https://github.com/halfirish83/builds/tree/master/1.7.0.2/Windows/deps

They are already built on windows, but sourcecode is included so that you can rebuild them if required.

Their licenses:

	OpenSSL        Old BSD license with the problematic advertising requirement
	Berkeley DB    New BSD license with additional requirement that linked software must be free open source
	Boost          MIT-like license
	miniupnpc      New (3-clause) BSD license

Versions used in this release:

	OpenSSL      1.0.1c
	Berkeley DB  4.8.30.NC
	Boost        1.50.0
	miniupnpc    1.6


build OpenSSL
-------
MSYS shell:

un-tar sources with MSYS 'tar xfz' to avoid issue with symlinks (OpenSSL ticket 2377)
change 'MAKE' env. variable from 'C:\MinGW32\bin\mingw32-make.exe' to '/c/MinGW32/bin/mingw32-make.exe'

	cd /c/openssl-1.0.1c-mgw
	./config
	make

Build Berkeley DB
-----------
MSYS shell:

	cd /c/db-4.8.30.NC-mgw/build_unix
	sh ../dist/configure --enable-mingw --enable-cxx
	make

Build Boost
-----
DOS prompt:

	downloaded boost jam 3.1.18
	cd \boost-1.50.0-mgw
	bjam toolset=gcc --build-type=complete stage

Build MiniUPnPc
---------
UPnP support is optional, make with `USE_UPNP=` to disable it.

MSYS shell:

	cd /c/miniupnpc-1.6-mgw
	make -f Makefile.mingw
	mkdir miniupnpc
	cp *.h miniupnpc/

Build Dimecoin
-------
DOS prompt:

	cd dimecoin
	qmake
	mingw32-make -f makefile.release
