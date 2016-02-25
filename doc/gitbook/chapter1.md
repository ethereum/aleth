# Installation

Installation is a different process dependent on which platform you run. At present, TurboEthereum supports three platforms: Ubuntu, Mac OS X and Windows.

For installing the desktop tools on Windows and Mac, just grab the [latest release](https://github.com/ethereum/webthree-umbrella/releases). (For Windows you might also need [this](http://www.microsoft.com/en-US/download/details.aspx?id=40784).)

For installing on Ubuntu or Homebrew, instructions follow.

# Installing on Ubuntu 14.04 and later (64-bit)

**Warning: The `ethereum-qt` PPA will upgrade your system-wide Qt5 installation, from 5.2 on Trusty and 5.3 on Utopic, to 5.5.**

For the latest stable version:
```
sudo add-apt-repository ppa:ethereum/ethereum-qt
sudo add-apt-repository ppa:ethereum/ethereum
sudo apt-get update
sudo apt-get install cpp-ethereum
```

If you want to use the cutting edge developer version:
```
sudo add-apt-repository ppa:ethereum/ethereum-qt
sudo add-apt-repository ppa:ethereum/ethereum
sudo add-apt-repository ppa:ethereum/ethereum-dev
sudo apt-get update
sudo apt-get install cpp-ethereum
```

## Installing the Mix IDE

Mix, the developer IDE is still in its infancy and still needs a lot of work. If you are adventurous, you can try to run Mix by installing the cutting edge developer version of cpp-ethereum (see above) and then add this:

```
sudo add-apt-repository ppa:ethereum/ethereum-qt
sudo add-apt-repository ppa:ethereum/ethereum
sudo add-apt-repository ppa:ethereum/ethereum-dev
sudo apt-get update
sudo apt-get install mix
mix
```

<!--
## Installing an Ethereum node server

To run a node server on Ubuntu, run:

```
wget http://opensecrecy.com/setupeth.sh && source ./setupeth.sh BRANCH NODE_IP NODE_NAME && rm -f setupeth.sh && reboot
```

- `BRANCH` should be substituted for either `master` or `develop`, depending on whether you want a stable or bleeding-edge version.
- `NODE_IP` should be substituted for the 4-digit, dot-deliminated IP address of the node. For example `1.2.3.4` or `192.168.1.69`.
- `NODE_NAME` should be substituted for the name of the node, quoted if it contains spaces. Avoid using symbols. e.g. `"Gavs Server Node"` or `Release_Node_1`.

Wait for it to reboot and you'll be running a node.
-->

# Installing on OS X and Homebrew

If you want the full suite of CLI tools, include `eth` and `ethminer`, you'll need [Homebrew](brew.sh). 

Once you've got Homebrew installed, tap the ethereum brew:
```
brew tap ethereum/ethereum
```

Then, for the stable version:
```
brew install cpp-ethereum
brew linkapps cpp-ethereum
```

or, for the latest cutting edge developer version:
```
brew reinstall cpp-ethereum --devel
brew linkapps cpp-ethereum
```

Add the `--with-gui` option to include the AlethZero and the Mix IDE in the installation; you can then find `AlethZero` and `Mix` in your Applications folder.

For options and patches, see: https://github.com/ethereum/homebrew-ethereum

