# Aleth: Ethereum C++ client and tools.
# Copyright 2018 Aleth Authors.
# Licensed under the GNU General Public License, Version 3. See the LICENSE file.

set(sources
    main.cpp

    external-dependencies/boost.cpp

    libdevcore/CommonJS.cpp
    libdevcore/core.cpp
    libdevcore/FixedHash.cpp
    libdevcore/RangeMask.cpp
    # FIXME: libdevcore/rlp.cpp

    libdevcrypto/AES.cpp
    libdevcrypto/crypto.cpp
    # FIXME: libdevcrypto/hexPrefix.cpp
    libdevcrypto/LibSnark.cpp
    # FIXME: libdevcrypto/MemTrie.cpp
    # FIXME: libdevcrypto/MemTrie.h
    # FIXME: libdevcrypto/SecretStore.cpp
    # FIXME: libdevcrypto/trie.cpp

    libethashseal/EthashTest.cpp
)

add_executable(aleth-test ${sources})
target_link_libraries(aleth-test PRIVATE ethashseal)

set_target_properties(aleth-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)
