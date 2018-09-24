# Aleth – Ethereum C++ client, tools and libraries

> The collection of C++ libraries and tools for Ethereum,
> formerly known as _cpp-ethereum_ project.
> This includes the full Ethereum client **aleth**.


## Contact

[![Gitter](https://img.shields.io/gitter/room/ethereum/aleth.svg)](https://gitter.im/ethereum/aleth)
[![GitHub Issues](https://img.shields.io/github/issues-raw/ethereum/aleth.svg)](https://github.com/ethereum/aleth/issues)

- Chat in [aleth channel on Gitter](https://gitter.im/ethereum/aleth).
- Report bugs, issues or feature requests using [GitHub issues](https://github.com/ethereum/aleth/issues/new).


## Getting Started

The Ethereum Documentation site hosts the **[aleth homepage](http://cpp-ethereum.org)**, which
has a Quick Start section.


Operating system | Status
---------------- | ----------
Ubuntu and macOS | [![TravisCI](https://img.shields.io/travis/ethereum/aleth/develop.svg)](https://travis-ci.org/ethereum/aleth)
Windows          | [![AppVeyor](https://img.shields.io/appveyor/ci/ethereum/cpp-ethereum/develop.svg)](https://ci.appveyor.com/project/ethereum/cpp-ethereum)


## Building from source

### Get the source code

Git and GitHub are used to maintain the source code. Clone the repository by:

```shell
git clone --recursive https://github.com/ethereum/aleth.git
cd aleth
```

The `--recursive` option is important. It orders git to clone additional
submodules to build the project.
If you missed `--recursive` option, you are able to correct your mistake with command
`git submodule update --init`.

### Install CMake

CMake is used to control the build configuration of the project. Latest version of CMake is required
(at the time of writing [3.4.3 is the minimum](CMakeLists.txt#L25)).
We strongly recommend you to install CMake by downloading and unpacking the binary
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
The examples for some systems are shown below:

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

We provide prebuilt dependencies to build the project. Download them
with the [scripts\install_deps.bat](scripts/install_deps.bat) script.

```shell
scripts\install_deps.bat
```

### Build

Configure the project build with the following command to create the
`build` directory with the configuration.

```shell
mkdir build; cd build  # Create a build directory.
cmake ..               # Configure the project.
cmake --build .        # Build all default targets.
```

On **Windows** Visual Studio 2015 is required. You should generate Visual Studio
solution file (.sln) for 64-bit architecture by adding
`-G "Visual Studio 14 2015 Win64"` argument to the CMake configure command.
After configuration is completed, the `aleth.sln` can be found in the
`build` directory.

```shell
cmake .. -G "Visual Studio 14 2015 Win64"
```
#### Common Issues Building on Windows
##### LINK : fatal error LNK1158: cannot run 'rc.exe'
Rc.exe is the [Microsoft Resource Compiler](https://docs.microsoft.com/en-us/windows/desktop/menurc/resource-compiler). It's distributed with the [Windows SDK](https://developer.microsoft.com/en-US/windows/downloads/windows-10-sdk) and is required for generating the Visual Studio solution file. It can be found in the following directory: ```%ProgramFiles(x86)%\Windows Kits\<OS major version>\bin\<OS full version>\<arch>\```

If you hit this error, adding the directory to your path (and launching a new command prompt) should fix the issue. 

## Contribute

[![Contributors](https://img.shields.io/github/contributors/ethereum/aleth.svg)](https://github.com/ethereum/aleth/graphs/contributors)
[![Gitter](https://img.shields.io/gitter/room/ethereum/aleth.svg)](https://gitter.im/ethereum/aleth)
[![up-for-grabs](https://img.shields.io/github/issues-raw/ethereum/aleth/help%20wanted.svg)](https://github.com/ethereum/aleth/labels/help%20wanted)

The current codebase is the work of many, many hands, with over 100
[individual contributors](https://github.com/ethereum/aleth/graphs/contributors) over the course of its development.

Our day-to-day development chat happens on the
[aleth](https://gitter.im/ethereum/aleth) Gitter channel.

All contributions are welcome! We try to keep a list of tasks that are suitable
for newcomers under the tag
[help wanted](https://github.com/ethereum/aleth/labels/help%20wanted).
If you have any questions, please do not hestitate to ask us about more information.

Please read [CONTRIBUTING](CONTRIBUTING.md) and [CODING_STYLE](CODING_STYLE.md)
thoroughly before making alterations to the code base.

All development goes in develop branch.


## Mining

This project is **not suitable for Ethereum mining** because the support for GPU mining
has been dropped some time ago including the ethminer tool. Use the ethminer tool from https://github.com/ethereum-mining/ethminer.

## Testing

To run the tests, make sure you clone https://github.com/ethereum/tests and point the environment variable
`ETHEREUM_TEST_PATH` to that path.

## Documentation

- [Internal documentation for developers](doc/index.rst).
- [Outdated documentation for end users](http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/).


## License

[![License](https://img.shields.io/github/license/ethereum/aleth.svg)](LICENSE)

All contributions are made under the [GNU General Public License v3](https://www.gnu.org/licenses/gpl-3.0.en.html). See [LICENSE](LICENSE).
