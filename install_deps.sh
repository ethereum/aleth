#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script for installing pre-requisite packages for cpp-ethereum on a
# variety of a Linux and other UNIX-derived platforms.
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
# The documentation for cpp-ethereum is hosted at:
#
# http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/
#
# (c) 2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

# Check for 'uname' and abort if it is not available.
uname -v > /dev/null 2>&1 || { echo >&2 "ERROR - cpp-ethereum requires 'uname' to identify the platform."; exit 1; }

case $(uname -s) in
    Darwin)
        case $(sw_vers -productVersion | awk -F . '{print $1"."$2}') in
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
                echo "We only support Yosemite and El Capitan, with work-in-progress on Sierra."
                exit 1
                ;;
        esac

        # Check for Homebrew install and abort if it is not installed.
        brew -v > /dev/null 2>&1 || { echo >&2 "ERROR - cpp-ethereum requires a Homebrew install.  See http://brew.sh."; exit 1; }

        brew update
        brew upgrade
        
        brew install boost
        brew install cmake
        brew install cryptopp
        brew install miniupnpc
        brew install leveldb
        brew install gmp
        brew install jsoncpp
        brew install libmicrohttpd
        brew install libjson-rpc-cpp

        # TODO - Need to update LLVM to 3.8, or, if that is already explicitly done
        # via the EVMJIT CMake changes, then just remove the macOS dependency here
        # and in the Homebrew formula.
        #
        # See https://github.com/ethereum/homebrew-ethereum/pull/67
        # See https://github.com/ethereum/evmjit/pull/53
        
        brew install homebrew/versions/llvm37

        ;;
    FreeBSD)
        echo "Installing cpp-ethereum dependencies on FreeBSD."
        echo "ERROR - 'install_deps.sh' doesn't have FreeBSD support yet."
        echo "Please let us know if you see this error message, and we can work out what is missing."
        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
        exit 1
        ;;
    Linux)
        case $(lsb_release -is) in
            Arch)
                #Arch
                echo "Installing cpp-ethereum dependencies on Arch Linux."
                echo "ERROR - 'install_deps.sh' doesn't have Arch Linux support yet."
                echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                echo "If you would like to get 'install_deps.sh' working for Arch Linux, that would be fantastic."
                echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                exit 1
                ;;
            Alpine)
                #Alpine
                echo "Installing cpp-ethereum dependencies on Alpine Linux."
                echo "ERROR - 'install_deps.sh' doesn't have Alpine Linux support yet."
                echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                echo "If you would like to get 'install_deps.sh' working for AlpineLinux, that would be fantastic."
                echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                echo "See also https://github.com/ethereum/webthree-umbrella/issues/495 where we are working through Alpine support."
                exit 1
                ;;
            Debian)
                #Debian
                case $(lsb_release -cs) in
                    jessie)
                        #jessie
                        echo "Installing cpp-ethereum dependencies on Debian Jesse (8.5)."
                        echo "ERROR - 'install_deps.sh' doesn't have Debian Jessie support yet."
                        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                        exit 1
                        ;;
                    *)
                        #other Debian
                        echo "Installing 'install_deps.sh' dependencies on unknown Debian version."
                        echo "ERROR - Debian Jessie is the only Debian version which cpp-ethereum has been tested on."
                        echo "If you are using a different release and would like to get 'install_deps.sh'"
                        echo "working for that release that would be fantastic."
                        echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                        exit 1
                        ;;
                esac
                ;;
            Fedora)
                #Fedora
                echo "Installing cpp-ethereum dependencies on Fedora."
                echo "ERROR - 'install_deps.sh' doesn't have Fedora support yet."
                echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                echo "If you would like to get CentOS working, that would be fantastic."
                echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                exit 1
                ;;
            "openSUSE project")
                #openSUSE
                echo "Installing cpp-ethereum dependencies on openSUSE."
                echo "ERROR - 'install_deps.sh' doesn't have openSUSE support yet."
                echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                echo "If you would like to get 'install_deps.sh' working for openSUSE, that would be fantastic."
                echo "See https://github.com/ethereum/webthree-umbrella/issues/552."
                exit 1
                ;;
            Ubuntu)
                #Ubuntu
                case $(lsb_release -cs) in
                    trusty)
                        #trusty
                        echo "Installing cpp-ethereum dependencies on Ubuntu Trusty Tahr (14.04)."
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
                    xenial)
                        #xenial
                        echo "Installing cpp-ethereum dependencies on Ubuntu Xenial Xerus (16.04)."
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
                # as we switch more to a "build from source" model, as Pawel has recently done for LLVM.
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
                #
                # TODO - Our Ubuntu build is only working for amd64 and i386 processors.
                # It would be good to add armel, armhf and arm64.
                # See https://github.com/ethereum/webthree-umbrella/issues/228.

                sudo add-apt-repository -y ppa:ethereum/ethereum
                sudo apt-get -y update
                sudo apt-get -y install \
                    build-essential \
                    cmake \
                    git \
                    libboost-all-dev \
                    libcurl4-openssl-dev \
                    libcryptopp-dev \
                    libgmp-dev \
                    libjsoncpp-dev \
                    libleveldb-dev \
                    libmicrohttpd-dev \
                    libminiupnpc-dev \
                    libz-dev \
                    mesa-common-dev \
                    ocl-icd-libopencl1 \
                    opencl-headers

                # The PPA also contains binaries for libjson-rpc-cpp-0.4.2, but we aren't actually using them.
                # Again, I think that will have been a workaround for libjsonrpccpp-dev not being
                # consistently available across Ubuntu releases.   Again, the package name differs
                # between our PPA and the official package repositories.
                #
                # - Not available for Trusty.
                # - http://packages.ubuntu.com/wily/libjsonrpccpp-dev (0.6.0-2)
                # - http://packages.ubuntu.com/xenial/libjsonrpccpp-dev (0.6.0-2)
                # - http://packages.ubuntu.com/yakkety/libjsonrpccpp-dev (0.6.0-2)
                #
                # Instead, we are building from source, grabbing the v0.6.0 tag from Github.
                #
                # That build-from-source appears to be an effective workaround for jsonrpcstub
                # silent failures at least within TravisCI runs on Ubuntu Trusty, but ...
                #
                # See https://github.com/ethereum/webthree-umbrella/issues/513
                #
                # Hmm.   Arachnid is still getting this issue on OS X, which already has v0.6.0, so
                # it isn't as simple as just updating all our builds to that version, though that is
                # sufficient for us to get CircleCI and TravisCI working.   We still haven't got to
                # the bottom of this issue, and are going to need to debug it in some scenario where
                # we can reproduce it 100%, which MIGHT end up being within our automation here, but
                # against a build-from-source-with-extra-printfs() of v0.4.2.

                sudo apt-get -y install libargtable2-dev libedit-dev
                git clone git://github.com/cinemast/libjson-rpc-cpp.git
                cd libjson-rpc-cpp
                git checkout v0.6.0
                mkdir build
                cd build
                cmake .. -DCOMPILE_TESTS=NO
                make
                sudo make install
                sudo ldconfig
                cd ../..

                # And install the English language package and reconfigure the locales.
                # We really shouldn't need to do this, and should instead force our locales to "C"
                # within our application runtimes, because this issue shows up on multiple Linux distros,
                # and each will need fixing in the install steps, where we should really just fix it once
                # in the code.
                #
                # See https://github.com/ethereum/webthree-umbrella/issues/169
                sudo apt-get -y install language-pack-en-base
                sudo dpkg-reconfigure locales

                ;;
            *)
                #other Linux
                echo "ERROR - Unsupported or unidentified Linux distro."
                echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                echo "If you would like to get your distro working, that would be fantastic."
                echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
                exit 1
                ;;
        esac
        ;;
    *)
        #other
        echo "ERROR - Unsupported or unidentified operating system."
        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/ for manual instructions."
        echo "If you would like to get your operating system working, that would be fantastic."
        echo "Drop us a message at https://gitter.im/ethereum/cpp-ethereum-development."
        ;;
esac
