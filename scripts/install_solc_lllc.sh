#!/usr/bin/env sh

# This script downloads the lllc/solc binaries and installs them in $PREFIX directory
# (the solc/lllc executables will be in $PREFIX/bin). By default $PREFIX is
# ~/.local but can we changes with --prefix <PREFIX> argument.

# This is mostly suitable for CIs, not end users.

set -e

VERSION=0.4.17

if [ "$1" = "--prefix" ]; then
    PREFIX="$2"
else
    PREFIX=~/.local
fi

SHA256=0f00df61c43215a82db7e9620d9bfaa964c63ff1f745ece62df89a7829ac9f7b


BIN=$PREFIX/bin

if test -f $BIN/lllc && ($BIN/lllc --version | grep -q "$VERSION"); then
    echo "CMake $VERSION already installed in $BIN"
else
    URL=https://github.com/ethereum/solidity/releases/download/v$VERSION/solidity-ubuntu-trusty.zip
    ERROR=0
    TMPFILE=$(mktemp --tmpdir solidity-$VERSION-$OS.XXXXXXXX.zip)
    echo "Downloading lllc ($URL)..."
    curl -L -s "$URL" > "$TMPFILE"

    if type -p sha256sum > /dev/null; then
        SHASUM="sha256sum"
    else
        SHASUM="shasum -a256"
    fi

    if ! ($SHASUM "$TMPFILE" | grep -q "$SHA256"); then
        echo "Checksum mismatch ($TMPFILE)"
        exit 1
    fi
    mkdir -p "$PREFIX"
    unzip $TMPFILE -d $BIN
    rm -rf $TMPFILE
fi
