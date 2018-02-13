# Building on Ubuntu
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
sudo apt-get install libssl-dev libminiupnpc-dev libqt4-dev libboost-all-dev
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
if you built the same version of BDB as above, just uncomment the following lines>
```
BOOST_LIB_PATH=/usr/local/BerkeleyDB.5.0/lib
BDB_INCLUDE_PATH=/usr/local/BerkeleyDB.5.0/include
```
Then start the build process:
```
cd ~/dimecoin/
qmake  #or use [qmake-qt4] if you got multiple versions of QT installed
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
make -f dimecoin.unix
```
