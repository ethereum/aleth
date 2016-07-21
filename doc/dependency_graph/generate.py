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
# The documentation for cpp-ethereum is hosted at:
#
# http://www.ethdocs.org/en/latest/ethereum-clients/cpp-ethereum/
#
# (c) 2015-2016 cpp-ethereum contributors.
#------------------------------------------------------------------------------

import os
import re

assignmentPattern = "(set|eth_name)\((EXECUTABLE|LIBRARY) (.*)\)"
dependencyPattern = "eth_use\((.*)\s+(OPTIONAL|REQUIRED)\s+(.*)\)"


# Returns a string which lists all the dependency edges for a given
# library or application, based on the declared OPTIONAL and REQUIRED
# dependencies within the CMake file for that library or application.
def getDependencyEdges(submodulePath, library):
    cmakeListsPath = os.path.join(os.path.join(submodulePath, library),
                                  "CMakeLists.txt")
    outputString = ""

    if os.path.exists(cmakeListsPath):
        executable = ""
        with open(cmakeListsPath) as fileHandle:
            for line in fileHandle.readlines():
                result = re.search(assignmentPattern, line)
                if result:
                    executable = result.group(3)
                result = re.search(dependencyPattern, line)
                if result:
                    fromNode = result.group(1)
                    if fromNode == "${EXECUTABLE}":
                        fromNode = executable
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
        if (subDirectoryName != "examples"):
            absSubDirectoryPath = os.path.join(submodulePath, subDirectoryName)
            if os.path.isdir(absSubDirectoryPath):
                cmakeListsPath = os.path.join(absSubDirectoryPath,
                                            "CMakeLists.txt")
                if os.path.exists(cmakeListsPath):
                    moduleName = ""
                    with open(cmakeListsPath) as fileHandle:
                        for line in fileHandle.readlines():
                            result = re.search(assignmentPattern, line)
                            if result:
                                moduleName = result.group(3)
                    if (moduleName == ""):
                        moduleName = subDirectoryName
                    outputString = outputString + '    "' + moduleName + '"'
                    outputString = outputString + "\n"
    return outputString


# Generate a sub-graph for each sub-module in the umbrella.
def processModule(root, folder):
    folderPath = os.path.join(root, folder)

    cleanName = folder.replace(".", "_").replace("-", "_")

    print "    subgraph cluster_" + cleanName + " {"
    print "        label = <" + folder + " dependencies>"
    print "        bgcolor = LavenderBlush"
    
    print getLibraryAndApplicationNames(folderPath)

    for module in os.listdir(folderPath):
        absLibPath = os.path.join(folderPath, module)
        if os.path.isdir(module):
            print getDependencyEdges(folderPath, module)

    print "    }"


# Walk the top-level folders within the repository
def processRepository(root):
    for folder in os.listdir(root):
        absPath = os.path.join(root, folder)
        if os.path.isdir(absPath):
            if not (".git" in absPath):
                folderPath = os.path.join(root, folder)
                print getLibraryAndApplicationNames(folderPath)
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
print "    subgraph cluster_external {"
print '        label = <https://github.com/ethereum/cpp-dependencies>'
print "        bgcolor = HoneyDew"
print '        "boost"'
print '        "curl"'
print '        "gmp"'
print '        "Jsoncpp"'
print '        "json-rpc-cpp"'
print '        "LevelDB"'
print '        "llvm"'
print '        "openssl"'
print '        "microhttpd"'
print '        "pthreads"'
print '        "ssh2"'
print '        "zlib"'
print "    }"
print '    "json-rpc-cpp" -> "curl"'
print '    "json-rpc-cpp" -> "microhttpd"'
print '    "json-rpc-cpp" -> "Jsoncpp"'
print '    "curl" -> "ssh2"  [style=dotted]'
print '    "curl" -> "openssl"  [style=dotted]'
print '    "curl" -> "zlib"  [style=dotted]'
print ''
print "    subgraph cluster_cppethereum {"
print '        label = <https://github.com/ethereum/cpp-ethereum>'
print "        bgcolor = LavenderBlush"
print '        "buildinfo"'
print '        "base"'
print '        "json_spirit" [color=red]'
print '        "scrypt" [color=red]'
print '        "secp256k1" [color=red]'

processRepository('../..')

print "    }"
print '    "base" -> "boost"'
print '    "base" -> "Jsoncpp"'
print '    "base" -> "json_spirit"'
print '    "base" -> "LevelDB"'
print '    "base" -> "pthreads"'
print '    "ethereum" -> "libevmjit" [style=dotted]'
print '    "libevmjit" -> "llvm"'
print '    "secp256k1" -> "gmp"'
print "}"
