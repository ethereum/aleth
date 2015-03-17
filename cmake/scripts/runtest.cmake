# Should be used to run ctest
# 
# example usage:
# cmake -DETH_TEST_NAME=TestInterfaceStub -P scripts/runtest.cmake


find_program(CTEST_COMMAND ctest)
if (NOT CTEST_COMMAND)
	message(FATAL_ERROR "ctest could not be found!")
endif()

execute_process(COMMAND ${CTEST_COMMAND} --force-new-ctest-process -C Debug -j 4 -V -R "${ETH_TEST_NAME}[.].*")

