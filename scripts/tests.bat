
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

    cd ..
    git clone --depth 1 https://github.com/ethereum/tests.git
    set ETHEREUM_TEST_PATH=%APPVEYOR_BUILD_FOLDER%\..\tests
    cd cpp-ethereum

    echo CONFIGURATION=%CONFIGURATION%
    echo APPVEYOR_BUILD_FOLDER=%APPVEYOR_BUILD_FOLDER%
    echo ETHEREUM_DEPS_PATH=%ETHEREUM_DEPS_PATH%

    cd build\test\libethereum\test\%CONFIGURATION%
    echo Testing testeth
    copy %APPVEYOR_BUILD_FOLDER%\build\evmjit\libevmjit\%CONFIGURATION%\evmjit.dll .
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll .
    testeth.exe || (echo "Failed" && exit /b 1)
    echo Testing EVMJIT
    testeth.exe -t VMTests,StateTests -- --vm jit --verbosity 2 || (echo "Failed" && exit /b 1)
    cd ..\..\..\..\..

    cd build\test\libweb3core\test\%CONFIGURATION%
    echo Testing testweb3core
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll .
    testweb3core.exe || (echo "Failed" && exit /b 1)
    cd ..\..\..\..\..

    cd build\test\webthree\test\%CONFIGURATION%
    echo Testing testweb3
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll .
    copy %ETHEREUM_DEPS_PATH%\win64\bin\OpenCL.dll .
    call testweb3.exe || (echo "Failed" && exit /b 1)
    cd ..\..\..\..\..
)
