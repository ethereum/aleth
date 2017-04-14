#!/usr/bin/env sh

set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    # In Travis Ubuntu 14.04 ccache is too old and does not support some of
    # the compiler flags and does not work. Upgrade it to version 3.2.
    curl -O http://de.archive.ubuntu.com/ubuntu/pool/main/c/ccache/ccache_3.2.4-1_amd64.deb
    sudo dpkg -i ccache_3.2.4-1_amd64.deb
fi

ccache --version
ccache --show-stats
ccache --zero-stats
ccache --max-size=1G
