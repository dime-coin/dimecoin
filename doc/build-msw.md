WINDOWS dimecoin wallet build instructions
===========================================

Compilers Supported
-------------------
g++ 4.6 or g++ 4.9 were for older versions of QT.
Since we need to compile with QT5 statically, we will use recent mingw-w64 compiler.
Download Msys2 here : http://mamedev.org/tools/
Choose the dual 32bit/64bit msys64_32-*.exe file to download

Prepare environment
-------------------
Extract msys64_32-*.exe on c:\
then in install directory c:\msys64, execute mingw32.exe for msys shell with path to 32bit mingw compiler or mingw64.exe for msys shell with path to 64bit compiler. We will use 32bit compiler in this guide.

In MSYS launched from Mingw32.exe for the first time, we need to download up to date packages :

	pacman -Sy
	pacman --needed -S bash pacman pacman-mirrors msys2-runtime

You must exit out from MSYS2-shell, restart MSYS2-shell, then run below command, to complete rest of other components update:

	pacman -Su

Download development packages :

	pacman -S base-devel git mercurial cvs wget p7zip
	pacman -S perl ruby python2 mingw-w64-i686-toolchain

Install QT 5
---------------
Obtain Pre-Built Qt files and Use instantly without Building/Compiling

	pacman -S mingw-w64-i686-qt5-static mingw-w64-i686-jasper 
	
One of dimecoin’s dependencies that will not need to be compiled cause static lib package available (currently 1.0.2n) :

	pacman -S mingw-w64-i686-openssl

Its license:

	OpenSSL        Old BSD license with the problematic advertising requirement

Dependencies
------------
Other dependencies will directly be compiled from source and placed here :

	C:\deps

They are already built on windows, but sourcecode is included so that you can rebuild them if required.

Their licenses:

	Berkeley DB    New BSD license with additional requirement that linked software must be free open source
	Boost          MIT-like license
	miniupnpc      New (3-clause) BSD license

Versions used in this release:

	Berkeley DB  5.0.32.NC
	Boost        1.58.0
	miniupnpc    1.9

Build Berkeley DB
-----------
Download Berkley DB 5.0.32 from here 
http://download.oracle.com/berkeley-db/db-5.0.32.NC.tar.gz
Put the archive in c:\deps.

MSYS shell launched from Mingw32.exe:

	cd /c/deps/
	tar xvfz db-5.0.32.NC.tar.gz

	cd /c/db-5.0.32.NC/build_unix
	sh ../dist/configure --enable-mingw --enable-cxx --disable-replication
	make

Build Boost
-----
Download Boost 1.58 here : https://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz/download
Put the archive in c:\deps

In Mingw32 Msys shell :

	cd /c/deps
	tar xvfz boost_1_58_0.tar.gz

From DOS prompt configured for MingW (launch c:\msys64\win32env.bat) :

	cd c:\deps\boost_1_58_0
	bootstrap.bat mingw
	b2 --build-type=complete --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc variant=release link=static threading=multi runtime-link=static stage

Build MiniUPnPc
---------
UPnP support is optional, make with `USE_UPNP=` to disable it.
Download it from here http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.9.tar.gz
and place it in your deps folder, next from the msys shell unpack it like this.

	cd /c/deps/
	tar xvfz miniupnpc-1.9.tar.gz

From DOS prompt configured for MingW (launch c:\msys64\win32env.bat) :

	cd c:\deps\miniupnpc-1.9
	make -f Makefile.mingw init upnpc-static
	
Important : then rename "miniupnpc-1.9" directory to "miniupnpc".

Build QREncode
---------
download 3.4.4 vereion here : https://fukuchi.org/works/qrencode/qrencode-3.4.4.tar.gz<br>
and put it in the deps directory before doing the following things in msys2 shell :
```
cd /c/deps
tar xvfz qrencode-3.4.4.tar.gz
cd qrencode-3.4.4
configure --enable-static --disable-shared --without-tools
make
```


Build Dimecoin
-------
First we need to adapt c:\dimecoin\dimecoin-qt.pro file to fit dependencies compiled above.
Comment all lines used for ubuntu compile.
Set DEPS_PATH to fit deps directory and adapt path of compiled dependencies under windows section :
```
windows {
	DEPS_PATH = /c/deps
	MINIUPNPC_LIB_PATH = $$DEPS_PATH/miniupnpc
	MINIUPNPC_INCLUDE_PATH = $$DEPS_PATH
	BOOST_LIB_PATH = $$DEPS_PATH/boost_1_58_0/stage/lib
	BOOST_INCLUDE_PATH = $$DEPS_PATH/boost_1_58_0
	BOOST_LIB_SUFFIX= -mgw73-mt-s-1_58
	BDB_LIB_PATH = $$DEPS_PATH/db-5.0.32.NC/build_unix
	BDB_INCLUDE_PATH = $$DEPS_PATH/db-5.0.32.NC/build_unix
#uncomment and modify below if you don't to want to use openssl static lib provided by mingw-w64 package
	#OPENSSL_LIB_PATH = $$DEPS_PATH/
	#OPENSSL_INCLUDE_PATH = $$DEPS_PATH/
	QRENCODE_INCLUDE_PATH= $$DEPS_PATH/qrencode-3.4.4
	QRENCODE_LIB_PATH= $$DEPS_PATH/qrencode-3.4.4/.libs
}
```
Then build leveldb, in MSYS shell executed from Mingw32.exe:

	cd /c/dimecoin/src/leveldb
	make clean
	TARGET_OS=NATIVE_WINDOWS make libleveldb.a libmemenv.a

then from Mingw32 Msys shell :

	export PATH=/mingw32/qt5-static/bin/:$PATH

	cd /c/dimecoin
	make clean
	qmake
	make -f Makefile.Release

NOTE : instructions for compiling 64bit windows wallet are the same except that :
- Mingw64.exe will be launched.
- packages name that were beginning with "mingw-w64-i686-" for 32bit env, must be replaced by "mingw-w64-x86_64-" when using pacman
- c:\msys64\win32\config64.bat must be executed before using c:\msys64\win32env.bat DOS prompt.
