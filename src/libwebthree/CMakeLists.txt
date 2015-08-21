cmake_policy(SET CMP0015 NEW)
set(CMAKE_AUTOMOC OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

include_directories(BEFORE ..)
include_directories(${DB_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

set(EXECUTABLE webthree)

file(GLOB HEADERS "*.h")

add_library(${EXECUTABLE} ${SRC_LIST} ${HEADERS})

target_link_libraries(${EXECUTABLE} ${ethereum_LIBRARY})
if (SERPENT)
	target_link_libraries(${EXECUTABLE} ${serpent_LIBRARY})
	add_definitions(-DETH_SERPENT)
endif()
if (SOLIDITY)
	target_link_libraries(${EXECUTABLE} ${solidity_LIBRARY})
	add_definitions(-DETH_SOLIDITY)
endif()
if (EVMJIT)
	target_link_libraries(${EXECUTABLE} ${evmjit_LIBRARY})
	add_definitions(-DETH_EVMJIT)
endif()
target_link_libraries(${EXECUTABLE} ${evm_LIBRARY})
target_link_libraries(${EXECUTABLE} ${lll_LIBRARY})
target_link_libraries(${EXECUTABLE} ${whisper_LIBRARY})
target_link_libraries(${EXECUTABLE} ${p2p_LIBRARY})
target_link_libraries(${EXECUTABLE} ${ethcore_LIBRARY})
target_link_libraries(${EXECUTABLE} ${devcrypto_LIBRARY})

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

