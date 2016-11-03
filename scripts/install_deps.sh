#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script for installing pre-requisite packages for cpp-ethereum on a
# variety of Linux and other UNIX-derived platforms.
#
# This is an "infrastucture-as-code" alternative to the manual build
# instructions pages which we previously maintained, first as Wiki pages
# and later as readthedocs pages at http://ethdocs.org.
#
# The aim of this script is to simplify things down to the following basic
# flow for all supported operating systems:
#
# - git clone --recursive
# - ./install_deps.sh
# - cmake && make
#
# At the time of writing we are assuming that 'lsb_release' is present for all
# Linux distros, which is not a valid assumption.  We will need a variety of
# approaches to actually get this working across all the distros which people
# are using.
#
# See http://unix.stackexchange.com/questions/92199/how-can-i-reliably-get-the-operating-systems-name
# for some more background on this common problem.
#
# TODO - There is no support here yet for cross-builds in any form, only
# native builds.  Expanding the functionality here to cover the mobile,
# wearable and SBC platforms covered by doublethink and EthEmbedded would
# also bring in support for Android, iOS, watchOS, tvOS, Tizen, Sailfish,
# Maemo, MeeGo and Yocto.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

set -e

# Check for 'uname' and abort if it is not available.
uname -v > /dev/null 2>&1 || { echo >&2 "ERROR - cpp-ethereum requires 'uname' to identify the platform."; exit 1; }

case $(uname -s) in

#------------------------------------------------------------------------------
# macOS
#------------------------------------------------------------------------------

    Darwin)
        case $(sw_vers -productVersion | awk -F . '{print $1"."$2}') in
            10.9)
                echo "Installing cpp-ethereum dependencies on OS X 10.9 Mavericks."
                ;;
            10.10)
                echo "Installing cpp-ethereum dependencies on OS X 10.10 Yosemite."
                ;;
            10.11)
                echo "Installing cpp-ethereum dependencies on OS X 10.11 El Capitan."
                ;;
            10.12)
                echo "Installing cpp-ethereum dependencies on macOS 10.12 Sierra."
                echo ""
                echo "NOTE - You are in unknown territory with this preview OS."
                echo "Even Homebrew doesn't have official support yet, and there are"
                echo "known issues (see https://github.com/ethereum/webthree-umbrella/issues/614)."
                echo "If you would like to partner with us to work through these issues, that"
                echo "would be fantastic.  Please just comment on that issue.  Thanks!"
                ;;
            *)
                echo "Unsupported macOS version."
                echo "We only support Mavericks, Yosemite and El Capitan, with work-in-progress on Sierra."
                exit 1
                ;;
        esac

        # Check for Homebrew install and abort if it is not installed.
        brew -v > /dev/null 2>&1 || { echo >&2 "ERROR - cpp-ethereum requires a Homebrew install.  See http://brew.sh."; exit 1; }

        # In August 2016, the 'carthage' package added a requirement for
        # a full Xcode 7.3 install, not just command-line build tools,
        # because it needs Swift 2.2.  This requirement is a complete
        # non-starter for TravisCI, where we have no ability or desire
        # to do that installation.  We aren't using 'carthage' ourselves,
        # so we just pin it here prior to 'brew update'.
        # https://github.com/Homebrew/homebrew-core/issues/3996
        brew pin carthage

        # A change was committed to 'brew' on 4th September 2016 which
        # broke various packages, including 'gnupg' and 'nvm.  We can
        # work around that issue by pinning the Formula for the time being.
        # The rolling release pattern strikes again.  Live projects
        # around the world are the test environment.
        brew pin gnupg
        brew pin nvm

        # Update Homebrew formulas and then upgrade any packages which
        # we have installed using these updated formulas.  This step is
        # required even within TravisCI, because the Homebrew formulas
        # are a constant moving target, and we always need to be chasing
        # those moving targets.  This is a fundamental design decision
        # made in 'rolling release' package management systems, and one
        # which makes our macOS builds fundamentally unstable and
        # unreliable.  We just had to try to react fast when anything
        # breaks.
        #
        # See https://github.com/ethereum/cpp-ethereum/issues/3089
        brew update
        brew upgrade

        # Bonus fun - TravisCI image for Yosemite includes a gmp version
        # which doesn't like being updated, so we need to uninstall it
        # first, so that the installation step below is a clean install.
        brew uninstall gmp

        # And finally install all the external dependencies.
        brew install \
            boost \
            ccache \
            cmake \
            cryptopp \
            gmp \
            leveldb \
            libmicrohttpd \
            miniupnpc

        ;;

#------------------------------------------------------------------------------
# FreeBSD
#------------------------------------------------------------------------------

    FreeBSD)
        echo "Installing cpp-ethereum dependencies on FreeBSD."
        echo "ERROR - 'install_deps.sh' doesn't have FreeBSD support yet."
        echo "Please let us know if you see this error message, and we can work out what is missing."
        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
        exit 1
        ;;

#------------------------------------------------------------------------------
# Linux
#------------------------------------------------------------------------------

    Linux)

#------------------------------------------------------------------------------
# Arch Linux
#------------------------------------------------------------------------------

        if [ -f "/etc/arch-release" ]; then

            echo "Installing cpp-ethereum dependencies on Arch Linux."

            # The majority of our dependencies can be found in the
            # Arch Linux official repositories.
            # See https://wiki.archlinux.org/index.php/Official_repositories
            pacman -Sy --noconfirm \
                autoconf \
                automake \
                gcc \
                libtool \
                boost \
                cmake \
                crypto++ \
                git \
                leveldb \
                libcl \
                libmicrohttpd \
                miniupnpc \
                opencl-headers

        fi

        case $(lsb_release -is) in

#------------------------------------------------------------------------------
# Alpine Linux
#------------------------------------------------------------------------------

            Alpine)
                #Alpine
                echo "Installing cpp-ethereum dependencies on Alpine Linux."
                echo "ERROR - 'install_deps.sh' doesn't have Alpine Linux support yet."
                echo "See http://cpp-ethereum.org/building-from-source/linux.html for manual instructions."
                echo "If you would like to get 'install_deps.sh' working for AlpineLinux, that would be fantastic."
                echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                echo "See also https://github.com/ethereum/webthree-umbrella/issues/495 where we are working through Alpine support."
                exit 1
                ;;

#------------------------------------------------------------------------------
# Debian
#------------------------------------------------------------------------------

            Debian)
                #Debian
                case $(lsb_release -cs) in
                    wheezy)
                        #wheezy
                        echo "Installing cpp-ethereum dependencies on Debian Wheezy (7.x)."
                        echo "See http://cpp-ethereum.org/building-from-source/linux.html for manual instructions."
                        echo "If you would like to get 'install_deps.sh' working for Debian Wheezy, that would be fantastic."
                        echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                        ;;
                    jessie)
                        #jessie
                        echo "Installing cpp-ethereum dependencies on Debian Jessie (8.x)."
                        ;;
                    stretch)
                        #stretch
                        echo "Installing cpp-ethereum dependencies on Debian Stretch (9.x)."
                        ;;
                    *)
                        #other Debian
                        echo "Installing 'install_deps.sh' dependencies on unknown Debian version."
                        echo "ERROR - Debian Jessie and Debian Stretch are the only Debian versions which cpp-ethereum has been tested on."
                        echo "If you are using a different release and would like to get 'install_deps.sh'"
                        echo "working for that release that would be fantastic."
                        echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                        exit 1
                        ;;
                esac

                # Install "normal packages"
                sudo apt-get -y update
                sudo apt-get -y install \
                    build-essential \
                    cmake \
                    g++ \
                    gcc \
                    git \
                    libboost-all-dev \
                    libcurl4-openssl-dev \
                    libgmp-dev \
                    libleveldb-dev \
                    libmicrohttpd-dev \
                    libminiupnpc-dev \
                    libz-dev \
                    mesa-common-dev \
                    ocl-icd-libopencl1 \
                    opencl-headers \
                    unzip

                # All the Debian releases until Stretch have shipped with CryptoPP 5.6.1,
                # but we need 5.6.2 or newer, so we build it from source.
                #
                # - https://packages.debian.org/wheezy/libcrypto++-dev (5.6.1)
                # - https://packages.debian.org/jessie/libcrypto++-dev (5.6.1)
                # - https://packages.debian.org/stretch/libcrypto++-dev (5.6.3)

                mkdir cryptopp && cd cryptopp
                wget https://www.cryptopp.com/cryptopp563.zip
                unzip -a cryptopp563.zip
                make dynamic
                make libcryptopp.so
                sudo make install PREFIX=/usr/local
                cd ..

                # All the Debian releases until Stretch have shipped with versions of CMake
                # which are too old for cpp-ethereum to build successfully.  CMake v3.0.x
                # should be the minimum version, but the v3.0.2 which comes with Jessie
                # doesn't work properly, so maybe our minimum version should actually be
                # CMake v3.1.x?  Anyway - we just build and install CMake from source
                # here, so that it works on any distro.
                #
                # - https://packages.debian.org/wheezy/cmake (2.8.9)
                # - https://packages.debian.org/jessie/cmake (3.0.2)
                # - https://packages.debian.org/stretch/cmake (3.5.2)

                wget https://cmake.org/files/v3.5/cmake-3.5.2.tar.gz
                tar -xf cmake-3.5.2.tar.gz
                cd cmake-3.5.2/
                cmake .
                ./bootstrap --prefix=/usr
                make -j 2
                sudo make install
                source ~/.profile
                cd ..
                rm -rf cmake-3.5.2
                rm cmake-3.5.2.tar.gz

                ;;

#------------------------------------------------------------------------------
# Fedora
#------------------------------------------------------------------------------

            Fedora)
                #Fedora
                echo "Installing cpp-ethereum dependencies on Fedora."

                # Install "normal packages"
                # See https://fedoraproject.org/wiki/Package_management_system.
                dnf install \
                    autoconf \
                    automake \
                    boost-devel \
                    cmake \
                    cryptopp-devel \
                    curl-devel \
                    gcc \
                    gcc-c++ \
                    git \
                    gmp-devel \
                    leveldb-devel \
                    libtool \
                    mesa-dri-drivers \
                    miniupnpc-devel \
                    snappy-devel

                ;;

#------------------------------------------------------------------------------
# OpenSUSE
#------------------------------------------------------------------------------

            "openSUSE project")
                #openSUSE
                echo "Installing cpp-ethereum dependencies on openSUSE."
                echo "ERROR - 'install_deps.sh' doesn't have openSUSE support yet."
                echo "See http://cpp-ethereum.org/building-from-source/linux.html for manual instructions."
                echo "If you would like to get 'install_deps.sh' working for openSUSE, that would be fantastic."
                echo "See https://github.com/ethereum/webthree-umbrella/issues/552."
                exit 1
                ;;

#------------------------------------------------------------------------------
# Ubuntu
#
# TODO - I wonder whether all of the Ubuntu-variants need some special
# treatment?
#
# TODO - We should also test this code on Ubuntu Server, Ubuntu Snappy Core
# and Ubuntu Phone.
#
# TODO - Our Ubuntu build is only working for amd64 and i386 processors.
# It would be good to add armel, armhf and arm64.
# See https://github.com/ethereum/webthree-umbrella/issues/228.
#------------------------------------------------------------------------------

            Ubuntu|LinuxMint)
                #Ubuntu or LinuxMint
                case $(lsb_release -cs) in
                    trusty|rosa|rafaela|rebecca|qiana)
                        #trusty or compatible LinuxMint distributions
                        echo "Installing cpp-ethereum dependencies on Ubuntu Trusty Tahr (14.04)."
                        echo "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main" \
                        | sudo tee -a /etc/apt/sources.list > /dev/null
                        ;;
                    utopic)
                        #utopic
                        echo "Installing cpp-ethereum dependencies on Ubuntu Utopic Unicorn (14.10)."
                        ;;
                    vivid)
                        #vivid
                        echo "Installing cpp-ethereum dependencies on Ubuntu Vivid Vervet (15.04)."
                        ;;
                    wily)
                        #wily
                        echo "Installing cpp-ethereum dependencies on Ubuntu Wily Werewolf (15.10)."
                        ;;
                    xenial|sarah)
                        #xenial
                        echo "Installing cpp-ethereum dependencies on Ubuntu Xenial Xerus (16.04)."
                        echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-3.9 main" \
                        | sudo tee -a /etc/apt/sources.list > /dev/null
                        ;;
                    yakkety)
                        #yakkety
                        echo "Installing cpp-ethereum dependencies on Ubuntu Yakkety Yak (16.10)."
                        echo ""
                        echo "NOTE - You are in unknown territory with this preview OS."
                        echo "We will need to update the Ethereum PPAs, work through build and runtime breaks, etc."
                        echo "See https://github.com/ethereum/webthree-umbrella/issues/624."
                        echo "If you would like to partner with us to work through these, that"
                        echo "would be fantastic.  Please just comment on that issue.  Thanks!"
                        ;;
                    *)
                        #other Ubuntu
                        echo "ERROR - Unknown or unsupported Ubuntu version."
                        echo "We only support Trusty, Utopic, Vivid, Wily and Xenial, with work-in-progress on Yakkety."
                        exit 1
                        ;;
                esac

                # The Ethereum PPA is required for the handful of packages where we need newer versions than
                # are shipped with Ubuntu itself.  We can likely minimize or remove the need for the PPA entirely
                # as we switch more to a "build from source" model, as Pawel has recently done for LLVM in evmjit.
                #
                # See https://launchpad.net/~ethereum/+archive/ubuntu/ethereum
                #
                # The version of CMake which shipped with Trusty was too old for our codebase (we need 3.0.0 or newer),
                # so the Ethereum PPA contains a newer release (3.2.2):
                #
                # - http://packages.ubuntu.com/trusty/cmake (2.8.12.2)
                # - http://packages.ubuntu.com/wily/cmake (3.2.2)
                # - http://packages.ubuntu.com/xenial/cmake (3.5.1)
                # - http://packages.ubuntu.com/yakkety/cmake (3.5.2)
                #
                # All the Ubuntu releases until Yakkety have shipped with CryptoPP 5.6.1, but we need 5.6.2
                # or newer.  Also worth of note is that the package name is libcryptopp in our PPA but
                # libcrypto++ in the official repositories.
                #
                # - http://packages.ubuntu.com/trusty/libcrypto++-dev (5.6.1)
                # - http://packages.ubuntu.com/wily/libcrypto++-dev (5.6.1)
                # - http://packages.ubuntu.com/xenial/libcrypto++-dev (5.6.1)
                # - http://packages.ubuntu.com/yakkety/libcrypto++-dev (5.6.3)
                #
                # NOTE - We actually want to remove the dependency in CryptoPP from our codebase entirely,
                # which would make this versioning problem moot.
                #
                # See https://github.com/ethereum/webthree-umbrella/issues/103

                if [ TRAVIS ]; then
                    # On Travis CI llvm package conficts with the new to be installed.
                    sudo apt-get -y remove llvm
                fi
                sudo apt-get -y update
                # this installs the add-apt-repository command
                sudo apt-get install -y --no-install-recommends software-properties-common
                sudo add-apt-repository -y ppa:ethereum/ethereum
                sudo apt-get -y update
                sudo apt-get install -y --no-install-recommends --allow-unauthenticated \
                    build-essential \
                    cmake \
                    git \
                    libboost-all-dev \
                    libcurl4-openssl-dev \
                    libcryptopp-dev \
                    libgmp-dev \
                    libleveldb-dev \
                    libmicrohttpd-dev \
                    libminiupnpc-dev \
                    libz-dev \
                    llvm-3.9-dev \
                    mesa-common-dev \
                    ocl-icd-libopencl1 \
                    opencl-headers

                ;;
            *)

#------------------------------------------------------------------------------
# Other (unknown) Linux
# Major and medium distros which we are missing would include Mint, CentOS,
# RHEL, Raspbian, Cygwin, OpenWrt, gNewSense, Trisquel and SteamOS.
#------------------------------------------------------------------------------

                #other Linux
                echo "ERROR - Unsupported or unidentified Linux distro."
                echo "See http://cpp-ethereum.org/building-from-source/linux.html for manual instructions."
                echo "If you would like to get your distro working, that would be fantastic."
                echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                exit 1
                ;;
        esac
        ;;

#------------------------------------------------------------------------------
# Other platform (not Linux, FreeBSD or macOS).
# Not sure what might end up here?
# Maybe OpenBSD, NetBSD, AIX, Solaris, HP-UX?
#------------------------------------------------------------------------------

    *)
        #other
        echo "ERROR - Unsupported or unidentified operating system."
        echo "See http://cpp-ethereum.org/building-from-source/ for manual instructions."
        echo "If you would like to get your operating system working, that would be fantastic."
        echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
        ;;
esac
