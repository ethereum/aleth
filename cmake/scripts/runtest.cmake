# Should be used to run ctest
# 
# example usage:
# cmake -DETH_TEST_NAME=TestInterfaceStub -P scripts/runtest.cmake

execute_process(COMMAND ctest --force-new-ctest-process -C Debug -j 1 -V -R ${ETH_TEST_NAME})

