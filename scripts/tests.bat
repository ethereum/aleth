
REM ---------------------------------------------------------------------------
REM Batch file for running the unit-tests for cpp-ethereum for Windows.
REM
REM The documentation for cpp-ethereum is hosted at:
REM
REM http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/
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

if "%TESTS%"=="On" (

    set CONFIGURATION=%2
    set REPO_ROOT=%3
    set ETHEREUM_DEPS_PATH=%4

    cd ..
    git clone https://github.com/ethereum/tests.git
    set ETHEREUM_TEST_PATH=%REPO_ROOT%\..\tests

    cd %REPO_ROOT%\build\test\libethereum\test\%CONFIGURATION%
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll .
    testeth.exe

    cd %REPO_ROOT%\build\test\libweb3core\test\%CONFIGURATION%
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll .
    testweb3core.exe

    cd %REPO_ROOT%\build\test\webthree\test\%CONFIGURATION%
    copy "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll .
    copy %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll .
    copy %ETHEREUM_DEPS_PATH%\win64\bin\OpenCL.dll .
    testweb3.exe
)
