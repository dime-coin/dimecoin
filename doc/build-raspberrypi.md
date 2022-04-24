RASPBERRY PI BUILD NOTES
====================
As of 4/19/2022 this build is tested and working on:<br>
<a href="https://github.com/dime-coin/dimecoin/commit/32df851c27452216a49cc394fc2238d64c74033d">Commit: 32df851c27452216a49cc394fc2238d64c74033d</a><br>
  Raspberry Pi 3 B Rev 1.2<br>
  Raspberry Pi 4 Model B Rev 1.2 (4GB)<br>
  Raspberry Pi 4 Model B Rev 1.4 (8GB)<br>
  OS Version: Raspbian GNU/Linux 11 (bullseye)<br>
  Kernel version(s):<br>
  Linux rpi3-64bit 5.15.32-v8+ #1538 SMP PREEMPT Thu Mar 31 19:40:39 BST 2022 aarch64 GNU/Linux<br>
  Linux rpi4-64bit-4gb 5.15.32-v8+ #1538 SMP PREEMPT Thu Mar 31 19:40:39 BST 2022 aarch64 GNU/Linux<br>
  gcc version(s):<br>
   10.2.1 20210110 (Debian 10.2.1-6)<br>
  
  (64bit OS and toolsets)<br>
  <br>
  32 bit OSs for the raspberry pi tends to run into syncing issues.
  For Raspberry Pi 3s, you will want to increase swap size.  2GB should be large enough to compile.

Increasing Swap Space for a Pi 3
------------------------------------
```
sudo dphys-swapfile swapoff
sudo nano /etc/dphys-swapfile
```
Find the line that says CONF_SWAPSIZE=100 and change the value to 2048 (2 GB)
Save and exit with ctrl+x.
Next the new sized swap file will need to be created and turned back on
```
sudo dphys-swapfile setup
sudo dphys-swapfile swapon
```
<br>

Build Instructions
------------------------------------
1. Make sure everything is up to date
```
sudo apt-get update
```

2. Download the Dimecoin source code:
```
git clone https://github.com/dime-coin/dimecoin.git
```

3. Install all the required libraries:
```
sudo apt-get install autoconf libtool libboost-all-dev openssl libssl-dev libcap-dev zlib1g-dev libbz2-dev g++-aarch64-linux-gnu curl binutils-aarch64-linux-gnu g++-arm-linux-gnueabihf binutils-arm-linux-gnueabihf binutils-gold bsdmainutils python-setuptools
```

4. Reboot the raspberry pi:
```
sudo reboot
```
<br><br>

Building dependencies
---------------------

5. Navigate to the contrib directory and build the Berkeley DB:
```
cd dimecoin/contrib
sudo ./install_db4.sh pwd
```

6. Change to the depends directory and compile for the linux arm 64bit tool set:
```
cd /home/pi/dimecoin/depends
make HOST=aarch64-linux-gnu -j4
```
This step will take about 2.5 hours to compile on a raspberry pi3.  It takes about 55 minutes to compile on a raspberry pi4.  If compiling on a raspberry pi3, it's highly recomended to not use the -j4.

Alternatvely if compiling on a 32bit OS use:
```
make HOST=arm-linux-gnueabihf -j4
```
<br><br>

Building Dimecoin Core
---------------------

7. change back to the dimecoin directory and run autogen CONFIG_SITE and ./configure
```
cd ..
./autogen.sh
CONFIG_SITE=$PWD/depends/aarch64-linux-gnu/share/config.site
./configure --prefix=`pwd`/depends/aarch64-linux-gnu
```

Alternatvely if compiling on a 32bit OS use:
```
CONFIG_SITE=$PWD/depends/arm-linux-gnueabihf/share/config.site
./configure --prefix=`pwd`/depends/arm-linux-gnueabihf
```

8. compile the dimecoin core:
```
make
```

This step will take about 3 hours to compile on a raspberry pi3.  It takes about 1 hour and 10 minutes to compile on a raspberry pi4.

9. Reboot the raspberry pi and you should be all finished.
```
sudo reboot
```
<br><br>

Starting up the daemon and/or qt wallet:
--------------------------
You can start the daemon up from the command line by navigating to the src directory:
```
cd dimecoin/src
./dimecoind
```

To start it up withing the UI, open a new terminal and navigate to the src/qt directory:
```
cd dimecoin/src/qt
./dimecoin-qt
```

It will take a few minutes to start up but should begin syncing with the network.

<br><br>

Using the Dimecoin testnet:
--------------------------
You can start the daemon up from the command line by navigating to the src directory:
```
cd dimecoin/src
./dimecoind -testnet
or
./dimecoind -testnet -addnode=159.223.112.164 
```
in separate terminal you can add some notes:

```
./dimecoin-cli -testnet addnode 159.223.112.164 onetry
```

To stop the daemon:
```
./dimecoin-cli -testnet stop
```

To run the qt wallet with testnet:
```
cd dimecoin/src/qt
./dimecoin-qt -testnet -addnode=159.223.112.164
```