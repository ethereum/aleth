
REM ---------------------------------------------------------------------------
REM Batch file for running the unit-tests for cpp-ethereum for Windows.
REM
REM The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
REM
REM ---------------------------------------------------------------------------
REM This file is part of cpp-ethereum.
REM
REM cpp-ethereum is free software: you can redistribute it and/or modify
REM it under the terms of the GNU General Public License as published by
REM the Free Software Foundation, either version 3 of the License, or
REM (at your option) any later version.
REM
REM cpp-ethereum is distributed in the hope that it will be useful,
REM but WITHOUT ANY WARRANTY; without even the implied warranty of
REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM GNU General Public License for more details.
REM
REM You should have received a copy of the GNU General Public License
REM along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
REM
REM Copyright (c) 2016 cpp-ethereum contributors.
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
