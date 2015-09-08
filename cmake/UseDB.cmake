function(eth_apply TARGET REQUIRED SUBMODULE)	

	if (${SUBMODULE} STREQUAL "auto")
		if (ROCKSDB_FOUND)
			eth_use(${TARGET} ${REQUIRED} DB::RocksDB)
		else ()
			eth_use(${TARGET} ${REQUIRED} DB::LevelDB)
		endif()
	endif()

	if (${SUBMODULE} STREQUAL "RocksDB")
		target_include_directories(${TARGET} SYSTEM PUBLIC ${ROCKSDB_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${ROCKSDB_LIBRARIES})
		target_compile_definitions(${TARGET} PUBLIC "ETH_ROCKSDB")
	endif()

	if (${SUBMODULE} STREQUAL "LevelDB")
		target_include_directories(${TARGET} SYSTEM PUBLIC ${LEVELDB_INCLUDE_DIRS})
		target_link_libraries(${TARGET} ${LEVELDB_LIBRARIES})
	endif()

endfunction()
