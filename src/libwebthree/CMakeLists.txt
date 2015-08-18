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

add_dependencies(${EXECUTABLE} BuildInfo.h)

target_link_libraries(${EXECUTABLE} ${ethereum_LIBRARY})
target_link_libraries(${EXECUTABLE} ${evm_LIBRARY})
target_link_libraries(${EXECUTABLE} ${lll_LIBRARY})
target_link_libraries(${EXECUTABLE} ${whisper_LIBRARY})
target_link_libraries(${EXECUTABLE} ${p2p_LIBRARY})
target_link_libraries(${EXECUTABLE} ${ethcore_LIBRARY})
target_link_libraries(${EXECUTABLE} ${devcrypto_LIBRARY})

install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin ARCHIVE DESTINATION lib LIBRARY DESTINATION lib )
install( FILES ${HEADERS} DESTINATION include/${EXECUTABLE} )

