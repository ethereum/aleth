REM get stuff!
if not exist download mkdir download
if not exist install mkdir install
if not exist install\windows mkdir install\windows

cd download
for /f "tokens=2 delims={}" %%g in ('bitsadmin /create nsis-3.0b1-setup.exe') do (
	bitsadmin /transfer {%%g} /download /priority normal https://build.ethdev.com/builds/windows-precompiled/nsis-3.0b1-setup.exe %cd%\nsis-3.0b1-setup.exe
	bitsadmin /cancel {%%g}
)
nsis-3.0b1-setup.exe /S
echo NSIS INSTALL: %ERRORLEVEL%
cd ..

if exist "C:\Program Files (x86)\NSIS\NSIS.exe" (
	echo "NSIS INSTALLED!!!!!"
)

set eth_server=https://build.ethdev.com/builds/windows-precompiled

call :download boost 1.55.0
call :download cryptopp 5.6.2
call :download curl 7.4.2
call :download jsoncpp 1.6.2
call :download json-rpc-cpp 0.5.0
call :download leveldb 1.2
call :download llvm 3.7svn
call :download microhttpd 0.9.2
call :download OpenCL_ICD 1
call :download qt 5.4.1
call :download miniupnpc 1.9
call :download v8 3.15.9

goto :EOF

:download

set eth_name=%1
set eth_version=%2

cd download

if not exist %eth_name%-%eth_version%.tar.gz (
	for /f "tokens=2 delims={}" %%g in ('bitsadmin /create %eth_name%-%eth_version%.tar.gz') do (
		bitsadmin /transfer {%%g} /download /priority normal %eth_server%/%eth_name%-%eth_version%.tar.gz %cd%\%eth_name%-%eth_version%.tar.gz
		bitsadmin /cancel {%%g}
	)
)
if not exist %eth_name%-%eth_version% cmake -E tar -zxvf %eth_name%-%eth_version%.tar.gz
cmake -E copy_directory %eth_name%-%eth_version% ..\install\windows

cd ..

goto :EOF
