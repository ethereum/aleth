# cpp-ethereum - Ethereum C++ client

This repository contains [cpp-ethereum](http://cpp-ethereum.org), the [Ethereum](https://ethereum.org) C++ client.

It is the third most popular of the Ethereum clients, behind [geth](https://github.com/ethereum/go-ethereum) (the [go](https://golang.org)
client) and [Parity](https://github.com/ethcore/parity) (the [rust](https://www.rust-lang.org/) client).  The code is exceptionally
[portable](http://cpp-ethereum.org/portability.html) and has been used successfully on a very broad range
of operating systems and hardware.


## Contact

[![Gitter](https://img.shields.io/gitter/room/ethereum/cpp-ethereum.svg)](https://gitter.im/ethereum/cpp-ethereum)
[![GitHub Issues](https://img.shields.io/github/issues-raw/ethereum/cpp-ethereum.svg)](https://github.com/ethereum/cpp-ethereum/issues)

- Chat in [cpp-ethereum channel on Gitter](https://gitter.im/ethereum/cpp-ethereum).
- Report bugs, issues or feature requests using [GitHub issues](issues/new).


## Getting Started

The Ethereum Documentation site hosts the **[cpp-ethereum homepage](http://cpp-ethereum.org)**, which
has a Quick Start section.


Operating system | Status
---------------- | ----------
Ubuntu and macOS | [![TravisCI](https://img.shields.io/travis/ethereum/cpp-ethereum/develop.svg)](https://travis-ci.org/ethereum/cpp-ethereum)
Windows          | [![AppVeyor](https://img.shields.io/appveyor/ci/ethereum/cpp-ethereum/develop.svg)](https://ci.appveyor.com/project/ethereum/cpp-ethereum)


## Building from source

### Get the source code

Git and GitHub is used to maintain the source code. Clone the repository by:

```shell
git clone --recursive https://github.com/ethereum/cpp-ethereum.git
cd cpp-ethereum
```

The `--recursive` option is important. It orders git to clone additional 
submodules which are required to build the project.
If you missed it you can correct your mistake with command 
`git submodule update --init`.

### Install CMake

CMake is used to control the build configuration of the project. Quite recent 
version of CMake is required 
(at the time of writing [3.4.3 is the minimum](CMakeLists.txt#L25)).
We recommend installing CMake by downloading and unpacking the binary 
distribution  of the latest version available on the 
[**CMake download page**](https://cmake.org/download/).

The CMake package available in your operating system can also be installed
and used if it meets the minimum version requirement.

> **Alternative method**
>
> The repository contains the
[scripts/install_cmake.sh](scripts/install_cmake.sh) script that downloads 
> a fixed version of CMake and unpacks it to the given directory prefix. 
> Example usage: `scripts/install_cmake.sh --prefix /usr/local`.

### Install dependencies (Linux, macOS)

The following *libraries* are required to be installed in the system in their
development variant:

- leveldb

They usually can be installed using system-specific package manager.
Examples for some systems:

Operating system | Installation command
---------------- | --------------------
Debian-based     | `sudo apt-get install libleveldb-dev`
RedHat-based     | `dnf install leveldb-devel`
macOS            | `brew install leveldb`


We also support a "one-button" shell script 
[scripts/install_deps.sh](scripts/install_deps.sh)
which attempts to aggregate dependencies installation instructions for Unix-like
operating systems. It identifies your distro and installs the external packages.
Supporting the script is non-trivial task so please [inform us](#contact)
if it does not work for your use-case.

### Install dependencies (Windows)

We provide prebuilt dependencies required to build the project. Download them
with the [scripts/install_deps.bat](scripts/install_deps.bat) script.

```shell
scripts/install_deps.bat
```

### Build

Configure the project build with the following command. It will create the 
`build` directory with the configuration.

```shell
mkdir build; cd build  # Create a build directory.
cmake ..               # Configure the project.
cmake --build .        # Build all default targets.
```

On **Windows** Visual Studio 2015 is required. You should generate Visual Studio 
solution file (.sln) for 64-bit architecture by adding 
`-G "Visual Studio 14 2015 Win64"` argument to the CMake configure command.
After configuration is completed the `cpp-ethereum.sln` can be found in the
`build` directory.

```shell
cmake .. -G "Visual Studio 14 2015 Win64"
```

## Contributing

[![Contributors](https://img.shields.io/github/contributors/ethereum/cpp-ethereum.svg)](https://github.com/ethereum/cpp-ethereum/graphs/contributors)
[![Gitter](https://img.shields.io/gitter/room/ethereum/cpp-ethereum.svg)](https://gitter.im/ethereum/cpp-ethereum)
[![up-for-grabs](https://img.shields.io/github/issues-raw/ethereum/cpp-ethereum/up-for-grabs.svg)](https://github.com/ethereum/cpp-ethereum/labels/up-for-grabs)

The current codebase is the work of many, many hands, with nearly 100
[individual contributors](https://github.com/ethereum/cpp-ethereum/graphs/contributors) over the course of its development.

Our day-to-day development chat happens on the
[cpp-ethereum](https://gitter.im/ethereum/cpp-ethereum) Gitter channel.

All contributions are welcome! We try to keep a list of tasks that are suitable
for newcomers under the tag 
[up-for-grabs](https://github.com/ethereum/cpp-ethereum/labels/up-for-grabs).
If you have any questions, please just ask.

Please read [CONTRIBUTING](CONTRIBUTING.md) and [CODING_STYLE](CODING_STYLE.md) 
thoroughly before making alterations to the code base.

All development goes in develop branch.


## Mining

This project is **not suitable for Ethereum mining**. The support for GPU mining 
has been dropped some time ago including the ethminer tool. Use the ethminer tool from https://github.com/ethereum-mining/ethminer.

## Testing

To run the tests, make sure you clone https://github.com/ethereum/tests and point the environment variable
`ETHEREUM_TEST_PATH` to that path.

## Documentation

- [Internal documentation for developers](doc/index.rst).
- [Outdated documentation for end users](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/).


## License

[![License](https://img.shields.io/github/license/ethereum/cpp-ethereum.svg)](LICENSE)

All contributions are made under the [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.en.html). See [LICENSE](LICENSE).
