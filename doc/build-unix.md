# Building on Ubuntu
The following instructions have been tested on Ubuntu Desktop 16.04

## Get the building environment ready

Open a terminal window. If git is not installed in your system, install it by issuing the following command
```
apt-get install git
```

Clone the dimecoin repository

```
git clone https://github.com/dime-coin/dimecoin.git
```

if required, fix the leveldb files permissions
```
cd ~/dimecoin/src/leveldb
chmod +x build_detect_platform
chmod 775 
```
you may also be required to build leveldb prior to start the wallet build
```
make clean
make libleveldb.a libmemenv.a
```

Install required libraries
```
apt-get istall libboost-dev libssl-dev miniupnpc libqt4-dev
```
Download and build BerkeleyDB 5.0.32.NC
```
cd ~/dimecoin/
wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
tar -xzvf db-5.0.32.NC.tar.gz
cd db-4.8.30.NC/build_unix/
../dist/configure --enable-cxx --disable-shared 
make install
```
Update dimecoin-qt.pro with the correct include and lib paths for BDB
```
cd ~/dimecoin/
nano dimecoin-qt.pro
```

## Build the qt wallet application (dimecoin-qt)
```
cd ~/dimecoin/
qmake
make
```

## Build the headless wallet (dimecoind)
```
cd ~/dimecoin/src
make -f dimecoin.unix
```
