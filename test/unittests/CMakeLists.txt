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

    libethcore/commonjs.cpp
    # FIXME: libethcore/difficulty.cpp
    libethcore/keymanager.cpp
    libethcore/PrecompiledTest.cpp
    libethcore/SealEngineTest.cpp
)

add_executable(aleth-test ${sources})
target_link_libraries(aleth-test PRIVATE ethashseal)

set_target_properties(aleth-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ..)

# Register unit test suites in ctest
set(suites "")
foreach(file ${sources})
    set(regex "BOOST_.+_TEST_SUITE\\(([a-zA-Z0-9]+)\\)")
    file(STRINGS ${file} suite_lines REGEX ${regex})
    foreach(line ${suite_lines})
        string(REGEX MATCH ${regex} _ ${line})
        list(APPEND suites ${CMAKE_MATCH_1})
        break()  # Only register the top suite.
    endforeach()
endforeach()

list(REMOVE_DUPLICATES suites)
foreach(suite ${suites})
    add_test(NAME aleth-test/${suite} COMMAND aleth-test -t ${suite}:!@bench)
endforeach()