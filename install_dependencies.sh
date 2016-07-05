#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script for installing pre-requisite packages for cpp-ethereum.
#
# The documentation for cpp-ethereum is hosted at:
#
# http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/
#
# (c) 2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

case $(uname -s) in
    Darwin)
        case $(sw_vers -productVersion | awk -F . '{print $1"."$2}') in
            10.10)
                echo "Installing cpp-ethereum dependencies on OS X 10.10 Yosemite"
                ;;
            10.11)
                echo "Installing cpp-ethereum dependencies on OS X 10.11 El Capitan"
                ;;
            10.12)
                echo "We don't have any support for macOS 10.12 Sierra yet."
                echo "If you are starting to use Sierra, and this is problematic, please let us know."
                echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                exit 1
                ;;
            *)
                echo "Unsupported macOS version.  We only support Yosemite and El Capitan."
                exit 1
                ;;
        esac

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
        brew install homebrew/versions/llvm37        
        ;;
    FreeBSD)
        echo "install_dependencies.sh doesn't have FreeBSD support yet."
        echo "Please let us know if you see this error message, and we can work out what is missing."
        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
        exit 1
        ;;
    Linux)
        case $(lsb_release -is) in
            Debian)
                #Debian
                case $(lsb_release -cs) in
                    jessie)
                        #jessie
                        echo "Debian Jessie"
                        echo "install_dependencies.sh doesn't have Debian Jessie support yet."
                        exit 1
                        ;;
                    *)
                        #other Debian
                        echo "Debian Jessie is the only Debian version which cpp-ethereum has been tested on."
                        echo "Please let us know if you see this error message, and we can work out what is missing."
                        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                        exit 1
                        ;;
                esac
                ;;
            Ubuntu)
                #Ubuntu
                case $(lsb_release -cs) in
                    trusty)
                        #trusty
                        echo "Installing cpp-ethereum dependencies on Ubuntu Trusty"

                        # Add additional PPAs which we need to be able to build cpp-ethereum on
                        # Ubuntu Trusty.  That includes our own PPAs and a PPA for getting CMake 3.x
                        # on Trusty.
                        sudo add-apt-repository -y ppa:ethereum/ethereum
                        sudo add-apt-repository -y ppa:ethereum/ethereum-dev
                        sudo apt-add-repository -y ppa:george-edison55/cmake-3.x
                        sudo apt-get -y update

                        # Install binaries for nearly all of our dependencies
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
                            opencl-headers

                        # The exception is libjson-rpc-cpp, which we have to build from source for
                        # reliable results.   The only binaries available for this package are those
                        # we made ourselves against the (now very old) v0.4.2 release, which are unreliable,
                        # so instead we build the latest release label (v0.6.0) from source, which works just
                        # fine.   We should update our PPA.
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
                    utopic)
                        #utopic
                        echo "install_dependencies.sh doesn't have Ubuntu Utopic support yet."
                        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux-ubuntu.html#ubuntu-utopic-unicorn-14-10 for manual steps."
                        exit 1
                        ;;
                    vivid)
                        #vivid
                        echo "install_dependencies.sh doesn't have Ubuntu Vivid support yet."
                        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux-ubuntu.html#ubuntu-vivid-vervet-15-04 for manual steps."
                        echo "Please let us know if you see this error message, and we can work out what is missing."
                        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                        exit 1
                        ;;
                    wily)
                        #wily
                        echo "install_dependencies.sh doesn't have Ubuntu Wily support yet."
                        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux-ubuntu.html#ubuntu-wily-werewolf-15-10 for manual steps."
                        echo "Please let us know if you see this error message, and we can work out what is missing."
                        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                        ;;
                    xenial)
                        #xenial
                        echo "install_dependencies.sh doesn't have Ubuntu Xenial support yet."
                        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux-ubuntu.html#ubuntu-xenial-xerus-16-04 for manual steps."
                        echo "Please let us know if you see this error message, and we can work out what is missing."
                        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                        ;;
                    yakkety)
                        #yakkety
                        echo "We don't have any support for Ubuntu Yakkety yet."
                        echo "If you are starting to use Yakkety, and this is problematic, please let us know."
                        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                        ;;
                    *)
                        #other Ubuntu
                        echo "Unsupported Ubuntu version.  We only support Trusty, Utopic, Vivid, Wily and Xenial."
                        exit 1
                        ;;
                esac
                ;;
            *)
                #other Linux
                echo "Unsupported or unidentified Linux distro."
                echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/linux.html for manual instructions."
                echo "Please let us know if you see this error message, and we can work out what is missing."
                echo "We had had success with Fedora, openSUSE and Alpine Linux too."
                echo "At https://gitter.im/ethereum/cpp-ethereum-development."
                ;;
        esac
        ;;
    *)
        #other
        echo "Unsupported or unidentified operating system."
        echo "See http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/building-from-source/ for manual instructions."
        echo "Please let us know if you see this error message, and we can work out what is missing."
        echo "At https://gitter.im/ethereum/cpp-ethereum-development."
        ;;
esac
