
aux_source_directory(. SRC_LIST)

macro (add_sources)
	foreach (_src ${ARGN})
		file (RELATIVE_PATH _relPath "${CMAKE_CURRENT_SOURCE_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}")
		list (APPEND SRC "./${_relPath}/${_src}")
		set(SRC ${SRC} PARENT_SCOPE)
	endforeach()
endmacro()

add_subdirectory(libtesteth)
add_subdirectory(fuzzTesting)
add_subdirectory(libethcore)
add_subdirectory(libethereum)
add_subdirectory(libtestutils)
add_subdirectory(libweb3core)
add_subdirectory(libdevcrypto)
add_subdirectory(libp2p)
add_subdirectory(libdevcore)
add_subdirectory(libweb3jsonrpc)
add_subdirectory(libwhisper)
add_subdirectory(libevm)
add_subdirectory(external-dependencies)

set(SRC_LIST ${SRC_LIST} ${SRC})

# search for test names and create ctest tests
enable_testing()
foreach(file ${SRC_LIST})
	file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/${file} test_list_raw REGEX "BOOST_.*TEST_(SUITE|CASE)")
	set(TestSuite "DEFAULT")
	foreach(test_raw ${test_list_raw})
		string(REGEX REPLACE ".*TEST_(SUITE|CASE)\\(([^ ,\\)]*).*" "\\1 \\2" test ${test_raw})
		if(test MATCHES "^SUITE .*")
			string(SUBSTRING ${test} 6 -1 TestSuite)
		elseif(test MATCHES "^CASE .*")
			string(SUBSTRING ${test} 5 -1 TestCase)
			add_test(NAME ${TestSuite}/${TestCase} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test COMMAND testeth -t ${TestSuite}/${TestCase})
		endif(test MATCHES "^SUITE .*")
	endforeach(test_raw)
endforeach(file)

file(GLOB_RECURSE HEADERS "*.h")
add_executable(testeth ${SRC_LIST} ${HEADERS})

find_package(Eth)
find_package(Web3)

target_include_directories(testeth PRIVATE ..)
target_include_directories(testeth PRIVATE ../utils)

target_link_libraries(testeth Boost::UnitTestFramework)
target_link_libraries(testeth ${Eth_TESTUTILS_LIBRARIES})

if (UNIX AND NOT APPLE)
	target_link_libraries(testeth pthread)
endif()

target_link_libraries(testeth ${Web3_WEB3JSONRPC_LIBRARIES})
target_link_libraries(testeth ${Web3_WEBTHREE_LIBRARIES})

enable_testing()
set(CTEST_OUTPUT_ON_FAILURE TRUE)

include(EthUtils)

eth_add_test(ClientBase
	ARGS --eth_testfile=BlockTests/bcJS_API_Test --eth_threads=1
	ARGS --eth_testfile=BlockTests/bcJS_API_Test --eth_threads=3
	ARGS --eth_testfile=BlockTests/bcJS_API_Test --eth_threads=10
	ARGS --eth_testfile=BlockTests/bcValidBlockTest --eth_threads=1
	ARGS --eth_testfile=BlockTests/bcValidBlockTest --eth_threads=3
	ARGS --eth_testfile=BlockTests/bcValidBlockTest --eth_threads=10
)

eth_add_test(JsonRpc
	ARGS --eth_testfile=BlockTests/bcJS_API_Test 
	ARGS --eth_testfile=BlockTests/bcValidBlockTest
)

