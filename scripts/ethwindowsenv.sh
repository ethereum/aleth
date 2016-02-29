#!/usr/bin/env bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# Our attempt to emulate vsvars32.bat in a bash environment for our windows bot
# Following suggestions from: http://stackoverflow.com/questions/366928/invoking-cl-exe-msvc-compiler-in-cygwin-shell

# These lines will be installation-dependent.
export VSINSTALLDIR='C:\Program Files (x86)\Microsoft Visual Studio 12.0\'
export WindowsSdkDir='C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\'
export FrameworkDir='C:\WINDOWS\Microsoft.NET\Framework64\'
export FrameworkDir64='C:\WINDOWS\Microsoft.NET\Framework64\'
export FrameworkVersion=v4.0.30319
export Framework40Version=v4.0
export Platform=X64

export WindowsLibPath='References\CommonConfiguration\Neutral'
export WindowsSDKLibVersion='winv6.3\'

# The following should be largely installation-independent.
export VCINSTALLDIR="$VSINSTALLDIR"'VC\'
export DevEnvDir="$VSINSTALLDIR"'Common7\IDE\'

export FrameworkDIR32="$FrameworkDir"
export FrameworkVersion32="$FrameworkVersion"
export FrameworkVersion64="$FrameworkVersion"

export INCLUDE="${VCINSTALLDIR}INCLUDE;${WindowsSdkDir}include;"
export LIB="${VCINSTALLDIR}LIB\amd64;${WindowsSdkDir}lib;"
export LIBPATH="${FrameworkDir}${FrameworkVersion};"
export LIBPATH="${LIBPATH}${FrameworkDir}${Framework40Version};"
export LIBPATH="${LIBPATH}${VCINSTALLDIR}LIB;"

c_VSINSTALLDIR=`cygpath -ua "$VSINSTALLDIR\\\\"`
c_WindowsSdkDir=`cygpath -ua "$WindowsSdkDir\\\\"`
c_FrameworkDir=`cygpath -ua "$FrameworkDir64\\\\"`

export PATH="${c_WindowsSdkDir}bin:$PATH"
export PATH="${c_WindowsSdkDir}bin/NETFX 4.0 Tools:$PATH"
export PATH="${c_VSINSTALLDIR}VC/VCPackages:$PATH"
export PATH="${c_FrameworkDir}${FrameworkVersion}:$PATH"
export PATH="${c_VSINSTALLDIR}Common7/Tools:$PATH"
export PATH="${c_VSINSTALLDIR}VC/BIN:$PATH"
export PATH="${c_VSINSTALLDIR}Common7/IDE/:$PATH"

MSBUILD_EXECUTABLE='/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin/amd64/msbuild.exe'
