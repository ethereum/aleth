
REM ---------------------------------------------------------------------------
REM Batch file for implementing release flow for cpp-ethereum for Windows.
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

set CONFIGURATION=%1
set ETHEREUM_DEPS_PATH=%2
set TESTS=%3
set TEST_EXECUTABLES=

if "%TESTS%"=="On" (
    TEST_EXECUTABLES= ^
        .\build\test\libethereum\test\%CONFIGURATION%\testeth.exe ^
        .\build\test\libweb3core\test\%CONFIGURATION%\testweb3core.exe ^
        .\build\test\webthree\test\%CONFIGURATION%\testweb3.exe
)

7z a cpp-ethereum-develop-windows.zip ^
    .\build\bench\%CONFIGURATION%\bench.exe ^
    .\build\eth\%CONFIGURATION%\eth.exe ^
    .\build\ethkey\%CONFIGURATION%\ethkey.exe ^
    .\build\ethminer\%CONFIGURATION%\ethminer.exe ^
    .\build\ethvm\%CONFIGURATION%\ethvm.exe ^
    .\build\rlp\%CONFIGURATION%\rlp.exe ^
    %TEST_EXECUTABLES% ^
    "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x86\Microsoft.VC140.CRT\msvc*.dll" ^
    %ETHEREUM_DEPS_PATH%\x64\bin\libcurl.dll ^
    %ETHEREUM_DEPS_PATH%\x64\bin\libmicrohttpd-dll.dll ^
    %ETHEREUM_DEPS_PATH%\win64\bin\OpenCL.dll
