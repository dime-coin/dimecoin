# 1) Building on Ubuntu with QT5 (dynamic linking)
The following instructions have been tested on the following distributions:

1. Ubuntu 16.04 (*)
2. Ubuntu 17.04
3. Mint 18.3

**(*) Please notice**: we experienced a issue on Ubuntu 16.04, where the wallet menu bar and tray icon was not shown due to an incompatibility with Unity window manager. This is not happening on Ubuntu 17.04 where Unity was replaced by Gnome.  

## Get the building environment ready

Open a terminal window. If git is not installed in your system, install it by issuing the following command
```
sudo apt-get install git
```
Install Linux development tools 
```
sudo apt-get install build-essential
```

Clone the dimecoin repository

```
git clone https://github.com/dime-coin/dimecoin.git
```

if required, fix the leveldb files permissions
```
cd ~/dimecoin/src/leveldb
chmod +x build_detect_platform
chmod 775 *
```
you may also be required to build leveldb prior to start the wallet build
```
make clean
make libleveldb.a libmemenv.a
```

Install required libraries
```
sudo apt-get install libssl-dev libminiupnpc-dev libboost-all-dev
```

To build with QT5, you will need these
```
sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler
```

Download and build BerkeleyDB 5.0.32.NC
```
cd ~/
wget 'http://download.oracle.com/berkeley-db/db-5.0.32.NC.tar.gz'
tar -xzvf db-5.0.32.NC.tar.gz
cd db-5.0.32.NC/build_unix/
../dist/configure --enable-cxx --disable-shared 
sudo make install
```
## Build the qt wallet application (dimecoin-qt)
Update dimecoin-qt.pro with the correct include and lib paths for BDB
```
cd ~/dimecoin/
nano dimecoin-qt.pro
```
if you built the same version of BDB as above, just add or modify the following lines under ubuntu build section
```
##############################################
# Uncomment to build on Ubuntu
##############################################
BOOST_LIB_PATH=/usr/local/BerkeleyDB.5.0/lib
BDB_INCLUDE_PATH=/usr/local/BerkeleyDB.5.0/include
```
Then start the build process:
```
cd ~/dimecoin/
qmake
make
```

## Build the headless wallet (dimecoind)
To export BDB environment variables to customize the location of BDB. If you used the same BDB version mentioned above, then type the following:
```
export BDB_INCLUDE_PATH=/usr/local/BerkeleyDB.5.0/include
export BDB_LIB_PATH=/usr/local/BerkeleyDB.5.0/lib
```
then start the build process:
```
cd ~/dimecoin/src
make -f makefile.unix
```

# 2) Building a static QT5 binary wallet on ubuntu
The following instructions have been tested on the following distributions:

1. Ubuntu 16.04.4
2. Linux Mint 18.3

The advantage of building the wallet this way is that it will be able to be executed even on a fresh ubuntu installation without adding additional libraries. There will be no dependencies error messages at startup in case some shared libs are missing. The current release was build that way.

## Get the building environment ready (same as above)

Open a terminal window. If git is not installed in your system, install it by issuing the following command
```
sudo apt-get install git
```
Install Linux development tools 
```
sudo apt-get install build-essential libtool automake autotools-dev autoconf pkg-config libgmp3-dev libevent-dev bsdmainutils
```
## Compile all dependencies manually and use their static libs
### Download and build BerkeleyDB 5.0.32.NC
```
cd ~/
wget 'http://download.oracle.com/berkeley-db/db-5.0.32.NC.tar.gz'
tar -xzvf db-5.0.32.NC.tar.gz
cd db-5.0.32.NC/build_unix/
../dist/configure --enable-cxx --disable-shared 
make

```

### Compiling Boost 1.58

Download Boost 1.58 here : 
https://sourceforge.net/projects/boost/files/boost/1.58.0/boost_1_58_0.tar.gz/download<br>
Put the archive in ~/deps

```
cd ~/deps
tar xvfz boost_1_58_0.tar.gz
cd ~/deps/boost_1_58_0
./bootstrap.sh

./b2 --build-type=complete --layout=versioned --with-chrono --with-filesystem --with-program_options --with-system --with-thread toolset=gcc variant=release link=static threading=multi runtime-link=static stage

```

### Compiling miniupnpc

Install Miniupnpc. Download it from here http://miniupnp.free.fr/files/download.php?file=miniupnpc-1.9.tar.gz<br>
and place it in your deps folder, then :
```
cd ~/deps
tar xvfz miniupnpc-1.9.tar.gz

cd miniupnpc-1.9
make init upnpc-static
```
==> Important : don't forget to rename "miniupnpc-1.9" directory to "miniupnpc"

### Compiling OpenSSL

download 1.0.2g version here : https://www.openssl.org/source/old/1.0.2/openssl-1.0.2g.tar.gz<br>
place archive in deps folders then :
```
tar xvfz openssl-1.0.2g.tar.gz
cd openssl-1.0.2g
./config no-shared no-dso
make depend
make
```

### Compiling QREncode

download 3.4.4 vereion here : https://fukuchi.org/works/qrencode/qrencode-3.4.4.tar.gz<br>
```
tar xvfz qrencode-3.4.4.tar.gz
cd qrencode-3.4.4
configure --enable-static --disable-shared --without-tools
make
```

### Compiling QT 5.5.0 statically
Download QT 5.5.0 sources
https://download.qt.io/archive/qt/5.5/5.5.0/single/qt-everywhere-opensource-src-5.5.0.tar.gz<br>
Extract in deps folder
```
tar xvfz qt-everywhere-opensource-src-5.5.0.tar.gz
```
after everything is extracted, create another directory where static libs will be installed. 
For example, i created ~/deps/Qt/5.4.2_static and used that directory in configure command below :
```
cd ~/deps/qt-everywhere-opensource-src-5.5.0

./configure -static -opensource -release -confirm-license -no-compile-examples -nomake tests -prefix ~/deps/Qt/5.5.0_static -qt-zlib -qt-libpng -no-libjpeg -qt-xcb -qt-freetype -qt-pcre -qt-harfbuzz -largefile -no-opengl -no-openssl -gtkstyle -skip wayland -skip qtserialport -skip script -skip qtdeclarative -pulseaudio -alsa -c++11 -nomake tools

make -j 4
```
After it successfuly ends :
```
make install
```
### Compiling Dimecoin QT wallet

Clone the dimecoin repository
```
git clone https://github.com/dime-coin/dimecoin.git
```
if required, fix the leveldb files permissions
```
cd ~/dimecoin/src/leveldb
chmod +x build_detect_platform
chmod 775 *
```
you may also be required to build leveldb prior to start the wallet build
```
make clean
make libleveldb.a libmemenv.a
```
go back to dimecoin dir to modify Dimecoin-qt.pro if needed :
```
cd ~/dimecoin
nano dimecoin-qt.pro
```
All dependencies dir variables to set according to what have been done above :
```
linux {
	DEPS_PATH = $(HOME)/deps
  ## comment below dependencies if u don't need to compile a static binary on linux
	MINIUPNPC_LIB_PATH = $$DEPS_PATH/miniupnpc
	MINIUPNPC_INCLUDE_PATH = $$DEPS_PATH
	BOOST_LIB_PATH = $$DEPS_PATH/boost_1_58_0/stage/lib
	BOOST_INCLUDE_PATH = $$DEPS_PATH/boost_1_58_0
	BDB_LIB_PATH = $$DEPS_PATH/db-5.0.32.NC/build_unix
	BDB_INCLUDE_PATH = $$DEPS_PATH/db-5.0.32.NC/build_unix
	OPENSSL_LIB_PATH = $$DEPS_PATH/openssl-1.0.2g
	OPENSSL_INCLUDE_PATH = $$DEPS_PATH/openssl-1.0.2g/include
	QRENCODE_INCLUDE_PATH= $$DEPS_PATH/qrencode-3.4.4
	QRENCODE_LIB_PATH= $$DEPS_PATH/qrencode-3.4.4/.libs
}
```
After saving the .pro file :
```
export PATH=$HOME/deps/Qt/5.5.0_static/bin:$PATH
qmake
make
```

Done!
