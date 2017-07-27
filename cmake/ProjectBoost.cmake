include(ExternalProject)
include(GNUInstallDirs)

set(BOOST_CXXFLAGS "")
if (WIN32)
    set(BOOST_BOOTSTRAP_COMMAND bootstrap.bat)
    set(BOOST_BUILD_TOOL b2.exe)
    set(BOOST_LIBRARY_SUFFIX -vc140-mt-1_63.lib)
else()
    set(BOOST_BOOTSTRAP_COMMAND ./bootstrap.sh)
    set(BOOST_BUILD_TOOL ./b2)
    set(BOOST_LIBRARY_SUFFIX .a)
    if (${BUILD_SHARED_LIBS})
        set(BOOST_CXXFLAGS "cxxflags=-fPIC")
    endif()
endif()

ExternalProject_Add(boost
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NO_PROGRESS 1
    URL https://github.com/ethereum/cpp-dependencies/releases/download/cache/boost_1_63_0.tar.gz
    URL_HASH SHA256=fe34a4e119798e10b8cc9e565b3b0284e9fd3977ec8a1b19586ad1dec397088b
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND ${BOOST_BOOTSTRAP_COMMAND}
    LOG_CONFIGURE 1
    BUILD_COMMAND 
        ${BOOST_BUILD_TOOL}
        --ignore-site-config
        stage
        ${BOOST_CXXFLAGS}
        threading=multi
        link=static
        variant=release
        address-model=64
        --with-chrono
        --with-date_time
        --with-filesystem
        --with-random
        --with-regex
        --with-test
        --with-thread
        --with-program_options  # libff wants it, we may need it in future.
    LOG_BUILD 1
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(boost SOURCE_DIR)
set(Boost_INCLUDE_DIR ${SOURCE_DIR})
set(boost_library_dir ${SOURCE_DIR}/stage/lib)
unset(BUILD_DIR)

add_library(Boost::chrono STATIC IMPORTED)
set_property(TARGET Boost::chrono PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::chrono PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_chrono${BOOST_LIBRARY_SUFFIX})
add_dependencies(Boost::chrono boost)

add_library(Boost::date_time STATIC IMPORTED)
set_property(TARGET Boost::date_time PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::date_time PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_date_time${BOOST_LIBRARY_SUFFIX})
add_dependencies(Boost::date_time boost)

add_library(Boost::regex STATIC IMPORTED)
set_property(TARGET Boost::regex PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::regex PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_regex${BOOST_LIBRARY_SUFFIX})
add_dependencies(Boost::regex boost)

add_library(Boost::system STATIC IMPORTED)
set_property(TARGET Boost::system PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::system PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_system${BOOST_LIBRARY_SUFFIX})
add_dependencies(Boost::system boost)

add_library(Boost::filesystem STATIC IMPORTED)
set_property(TARGET Boost::filesystem PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::filesystem PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_filesystem${BOOST_LIBRARY_SUFFIX})
set_property(TARGET Boost::filesystem PROPERTY INTERFACE_LINK_LIBRARIES Boost::system)
add_dependencies(Boost::filesystem boost)

add_library(Boost::random STATIC IMPORTED)
set_property(TARGET Boost::random PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::random PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_random${BOOST_LIBRARY_SUFFIX})
add_dependencies(Boost::random boost)

add_library(Boost::unit_test_framework STATIC IMPORTED)
set_property(TARGET Boost::unit_test_framework PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::unit_test_framework PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_unit_test_framework${BOOST_LIBRARY_SUFFIX})
add_dependencies(Boost::unit_test_framework boost)

add_library(Boost::thread STATIC IMPORTED)
set_property(TARGET Boost::thread PROPERTY IMPORTED_CONFIGURATIONS Release)
set_property(TARGET Boost::thread PROPERTY IMPORTED_LOCATION_RELEASE ${boost_library_dir}/libboost_thread${BOOST_LIBRARY_SUFFIX})
set_property(TARGET Boost::thread PROPERTY INTERFACE_LINK_LIBRARIES Boost::chrono Boost::date_time Boost::regex)
add_dependencies(Boost::thread boost)
