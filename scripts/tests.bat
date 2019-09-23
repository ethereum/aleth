
REM ---------------------------------------------------------------------------
REM Batch file for running the unit-tests for cpp-ethereum for Windows.
REM
REM The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
REM
REM ---------------------------------------------------------------------------
REM Aleth: Ethereum C++ client, tools and libraries.
REM Copyright 2019 Aleth Authors.
REM Licensed under the GNU General Public License, Version 3.
REM ---------------------------------------------------------------------------

set TESTS=%1
set CONFIGURATION=%2
set APPVEYOR_BUILD_FOLDER=%cd%
set ETHEREUM_DEPS_PATH=%4

if "%TESTS%"=="On" (

    REM Copy the DLLs into the test directory which need to be able to run.
    cd %APPVEYOR_BUILD_FOLDER%\build\test\%CONFIGURATION%
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .

    REM Run the tests for the Interpreter
    echo Testing testeth
    testeth.exe -- --testpath %APPVEYOR_BUILD_FOLDER%\test\jsontests
    IF errorlevel 1 GOTO ON-ERROR-CONDITION

    REM Run the tests for the JIT
    REM echo Testing EVMJIT
    REM testeth.exe -t VMTests,StateTests -- --vm jit --testpath %APPVEYOR_BUILD_FOLDER%\test\jsontests
    REM IF errorlevel 1 GOTO ON-ERROR-CONDITION

    cd ..\..\..

)

EXIT 0

:ON-ERROR-CONDITION
echo "ERROR - Unit-test run returned error code."
EXIT 1
