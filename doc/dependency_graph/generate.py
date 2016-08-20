#!/usr/bin/env python

# ------------------------------------------------------------------------------
# This file is part of cpp-ethereum.
#
# cpp-ethereum is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# cpp-ethereum is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>
#
#------------------------------------------------------------------------------
# Python script to generate a DOT graph showing the dependency graph of
# the modules within the cpp-ethereum project.   It is a mixture of
# dynamically-generated and hard-coded content to augment and improve
# the results.
#
# The script was originally written by Bob Summerwill to assist his own
# efforts at understanding the dependencies for the cpp-ethereum-cross
# project which cross-builds cpp-ethereum, and then contributed to the
# main cpp-ethereum project.
#
# See https://github.com/doublethinkco/cpp-ethereum-cross for more
# information on the cross-builds project.
#
# The documentation for cpp-ethereum is hosted at http://cpp-ethereum.org
#
# (c) 2015-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

import os
import re

dependencyPattern = "eth_use\((.*)\s+(OPTIONAL|REQUIRED)\s+(.*)\)"


# Returns a string which lists all the dependency edges for a given
# library or application, based on the declared OPTIONAL and REQUIRED
# dependencies within the CMake file for that library or application.
def getDependencyEdges(submodulePath, library):
    cmakeListsPath = os.path.join(os.path.join(submodulePath, library),
                                  "CMakeLists.txt")
    outputString = ""

    if os.path.exists(cmakeListsPath):
        with open(cmakeListsPath) as fileHandle:
            for line in fileHandle.readlines():
                result = re.search(dependencyPattern, line)
                if result:
                    fromNode = result.group(1)
                    toNodes = result.group(3).split()
                    for toNode in toNodes:
                        # Merge all JsonRpc::* nodes to simplify the output graph.
                        # Not much value in the details there.
                        if toNode.startswith("JsonRpc::"):
                            toNode = "json-rpc-cpp"
                        elif "::" in toNode:
                            toNode = toNode.split("::")[1]
                        edgeText = '"' + fromNode + '" -> "' + toNode + '"'
                        if "OPTIONAL" in line:
                            edgeText = edgeText + " [style=dotted]"
                        outputString = outputString + edgeText + "\n"

    return outputString


# Return a string which is a list of all the library and application
# names within a given git sub-module directory.
def getLibraryAndApplicationNames(submodulePath):
    outputString = ""
    for subDirectoryName in os.listdir(submodulePath):
        if (subDirectoryName != "examples" and subDirectoryName != "utils" and subDirectoryName != ".git"):
            absSubDirectoryPath = os.path.join(submodulePath, subDirectoryName)
            if os.path.isdir(absSubDirectoryPath):
                cmakeListsPath = os.path.join(absSubDirectoryPath,
                                            "CMakeLists.txt")
                if os.path.exists(cmakeListsPath):
                    moduleName = subDirectoryName
                    
                    if (moduleName == "libdevcore"):
                        moduleName = "devcore"
                    if (moduleName == "libdevcrypto"):
                        moduleName = "devcrypto"
                    if (moduleName == "libethash"):
                        moduleName = "ethash"
                    if (moduleName == "libethash-cl"):
                        moduleName = "ethash-cl"
                    if (moduleName == "libethashseal"):
                        moduleName = "ethashseal"
                    if (moduleName == "libethcore"):
                        moduleName = "ethcore"
                    if (moduleName == "libethereum"):
                        moduleName = "ethereum"
                    if (moduleName == "libevm"):
                        moduleName = "evm"
                    if (moduleName == "libevmcore"):
                        moduleName = "evmcore"
                    if (moduleName == "libp2p"):
                        moduleName = "p2p"
                    if (moduleName == "libwebthree"):
                        moduleName = "webthree"
                    if (moduleName == "libweb3jsonrpc"):
                        moduleName = "web3jsonrpc"
                    if (moduleName == "libwhisper"):
                        moduleName = "whisper"

                    outputString = outputString + '    "' + moduleName + '"'
                    
                    if (moduleName == "evmjit"):
                        outputString = outputString + " [style=filled,fillcolor=coral]"
                    elif ("lib" in absSubDirectoryPath):
                        outputString = outputString + " [style=filled,fillcolor=deepskyblue]"
                    else:
                        outputString = outputString + " [shape=box;style=filled,penwidth=2,fillcolor=chartreuse]"
                    
                    outputString = outputString + "\n"
    return outputString

# Walk the top-level folders within the repository
def processRepository(root):
    print getLibraryAndApplicationNames(root)
    for folder in os.listdir(root):
        absPath = os.path.join(root, folder)
        if os.path.isdir(absPath):
            if not (".git" in absPath):
                print getDependencyEdges(root, folder)


print 'digraph webthree {'
print '    graph [ label   = "Ethereum C++ dependencies" ]'
print '    node  [ fontname = "Courier", fontsize = 10 ]'
print ''
print '    compound = true'
print ''
print '    "buildinfo" [style=filled,fillcolor=deepskyblue]'
print '    "json_spirit" [color=red]'
print '    "scrypt" [color=red]'
print '    "secp256k1" [color=red]'
print '    "testeth" [shape=box;style=filled,penwidth=2,fillcolor=chartreuse]'
print '    "testweb3" [shape=box;style=filled,penwidth=2,fillcolor=chartreuse]'
print '    "testweb3core" [shape=box;style=filled,penwidth=2,fillcolor=chartreuse]'
print ''
print '    "bench" -> "devcrypto"'
print '    "curl" -> "ssh2"  [style=dotted]'
print '    "curl" -> "openssl"  [style=dotted]'
print '    "curl" -> "zlib"  [style=dotted]'
print '    "devcore" -> "boost::filesystem"'
print '    "devcore" -> "boost::random"'
print '    "devcore" -> "boost::system"'
print '    "devcore" -> "boost::thread"'
print '    "devcore" -> "LevelDB"'
print '    "devcrypto" -> "devcore"'
print '    "devcrypto" -> "json_spirit"'
print '    "devcrypto" -> "scrypt"'
print '    "devcrypto" -> "secp256k1"'
print '    "eth" -> "web3jsonrpc"'
print '    "ethcore" -> "devcore"'
print '    "ethcore" -> "buildinfo"'
print '    "ethcore" -> "json_spirit"'
print '    "ethereum" -> "boost::regex"'
print '    "ethereum" -> "evm"'
print '    "ethereum" -> "evmjit" [style=dotted]'
print '    "ethereum" -> "json_spirit"'
print '    "ethereum" -> "p2p"'
print '    "ethash" -> "Cryptopp"'
print '    "ethash-cl" -> "ethash"'
print '    "ethashseal" -> "ethereum"'
print '    "ethkey" -> "ethcore"'
print '    "ethminer" -> "ethashseal"'
print '    "ethvm" -> "ethashseal"'
print '    "evm" -> "ethcore"'
print '    "evm" -> "evmcore"'
print '    "json-rpc-cpp" -> "argtable2" [style=dotted]'
print '    "json-rpc-cpp" -> "curl"'
print '    "json-rpc-cpp" -> "microhttpd"'
print '    "json-rpc-cpp" -> "Jsoncpp"'
print '    "json-rpc-cpp" -> "libedit"'
print '    "LevelDB" -> "snappy" [style=dotted]'
print '    "evmjit" -> "llvm"'
print '    "p2p" -> "devcrypto"'
print '    "rlp" -> "devcrypto"'
print '    "rlp" -> "json_spirit"'
print '    "secp256k1" -> "gmp"'
print '    "testeth" -> "ethashseal"'
print '    "testeth" -> "boost::unit_test_framework"'
print '    "testweb3" -> "boost::unit_test_framework"'
print '    "testweb3" -> "web3jsonrpc"'
print '    "testweb3core" -> "boost::date_time"'
print '    "testweb3core" -> "boost::regex"'
print '    "testweb3core" -> "boost::unit_test_framework"'
print '    "testweb3core" -> "devcore"'
print '    "testweb3core" -> "devcrypto"'
print '    "testweb3core" -> "p2p"'
print '    "webthree" -> "ethashseal"'
print '    "webthree" -> "whisper"'
print '    "web3jsonrpc" -> "webthree"'
print '    "whisper" -> "boost::regex"'
print '    "whisper" -> "p2p"'
print ''

processRepository('../..')

print "}"
