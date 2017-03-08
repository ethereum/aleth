include(ExternalProject)
include(GNUInstallDirs)

ExternalProject_Add(cryptopp
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    # This points to unreleased version 5.6.5+ but contains very small
    # warning fix:
    # https://github.com/weidai11/cryptopp/commit/903b8feaa70199eb39a313b32a71268745ddb600
    DOWNLOAD_NAME cryptopp_bccc6443.tar.gz
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/weidai11/cryptopp/archive/bccc6443c4d4d611066c2de4c17109380cf97704.tar.gz
    URL_HASH SHA256=f1fddacadd2a0873f795d5614a85fecd5b6ff1d1c6e21dedc251703c54ce63aa
    PATCH_COMMAND ${CMAKE_COMMAND} -E remove
        3way.cpp
        adler32.cpp
        # algebra.cpp
        # algparam.cpp
        arc4.cpp
        # asn.cpp
        authenc.cpp
        base32.cpp
        base64.cpp
        # basecode.cpp
        bench1.cpp
        bench2.cpp
        bfinit.cpp
        blake2.cpp
        blowfish.cpp
        blumshub.cpp
        camellia.cpp
        cast.cpp
        casts.cpp
        cbcmac.cpp
        ccm.cpp
        chacha.cpp
        channels.cpp
        cmac.cpp
        # cpu.cpp
        crc.cpp
        # cryptlib.cpp
        datatest.cpp
        default.cpp
        des.cpp
        dessp.cpp
        dh2.cpp
        dh.cpp
        # dll.cpp
        dlltest.cpp
        # dsa.cpp
        eax.cpp
        # ec2n.cpp
        # eccrypto.cpp
        # ecp.cpp
        elgamal.cpp
        emsa2.cpp
        # eprecomp.cpp
        esign.cpp
        files.cpp
        # filters.cpp
        # fips140.cpp
        fipsalgt.cpp
        fipstest.cpp
        gcm.cpp
        gf2_32.cpp
        gf256.cpp
        # gf2n.cpp
        # gfpcrypt.cpp
        gost.cpp
        gzip.cpp
        # hex.cpp
        # hmac.cpp
        # hrtimer.cpp
        ida.cpp
        idea.cpp
        # integer.cpp
        # iterhash.cpp
        # keccak.cpp
        luc.cpp
        mars.cpp
        marss.cpp
        md2.cpp
        md4.cpp
        md5.cpp
        # misc.cpp
        # modes.cpp
        # mqueue.cpp
        mqv.cpp
        # nbtheory.cpp
        network.cpp
        # oaep.cpp
        # osrng.cpp
        panama.cpp
        pch.cpp
        pkcspad.cpp
        poly1305.cpp
        # polynomi.cpp
        pssr.cpp
        # pubkey.cpp
        # queue.cpp
        rabin.cpp
        # randpool.cpp
        rc2.cpp
        rc5.cpp
        rc6.cpp
        rdrand.cpp
        # rdtables.cpp
        regtest.cpp
        # rijndael.cpp
        ripemd.cpp
        # rng.cpp
        rsa.cpp
        rw.cpp
        safer.cpp
        salsa.cpp
        seal.cpp
        seed.cpp
        serpent.cpp
        sha3.cpp
        shacal2.cpp
        # sha.cpp
        sharkbox.cpp
        shark.cpp
        simple.cpp
        skipjack.cpp
        socketft.cpp
        sosemanuk.cpp
        square.cpp
        squaretb.cpp
        # strciphr.cpp
        tea.cpp
        test.cpp
        tftables.cpp
        tiger.cpp
        tigertab.cpp
        trdlocal.cpp
        ttmac.cpp
        twofish.cpp
        validat0.cpp
        validat1.cpp
        validat2.cpp
        validat3.cpp
        vmac.cpp
        wait.cpp
        wake.cpp
        whrlpool.cpp
        # winpipes.cpp
        xtr.cpp
        xtrcrypt.cpp
        zdeflate.cpp
        zinflate.cpp
        zlib.cpp
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DCMAKE_BUILD_TYPE=Release
        # Build static lib but suitable to be included in a shared lib.
        -DCMAKE_POSITION_INDEPENDENT_CODE=${BUILD_SHARED_LIBS}
        -DBUILD_SHARED=Off
        -DBUILD_TESTING=Off
    LOG_CONFIGURE 1
    # Overwrite build and install commands to force Release build on MSVC.
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release
    INSTALL_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target install
    LOG_INSTALL 1
)

# Create cryptopp imported library
ExternalProject_Get_Property(cryptopp INSTALL_DIR)
add_library(Cryptopp STATIC IMPORTED)
if (MSVC)
    set(CRYPTOPP_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}cryptopp-static${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
    set(CRYPTOPP_LIBRARY ${INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}cryptopp${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()
set(CRYPTOPP_INCLUDE_DIR ${INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${CRYPTOPP_INCLUDE_DIR})  # Must exist.
set_property(TARGET Cryptopp PROPERTY IMPORTED_LOCATION ${CRYPTOPP_LIBRARY})
set_property(TARGET Cryptopp PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CRYPTOPP_INCLUDE_DIR})
add_dependencies(Cryptopp cryptopp)
unset(INSTALL_DIR)
