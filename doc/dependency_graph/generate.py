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
        if (subDirectoryName != "examples" and subDirectoryName != "utils" and subDirectoryName != ".git"):
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
                    
                    if ("lib" in absSubDirectoryPath or (moduleName == "evmjit")):
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
print '    "base" [style=filled,fillcolor=deepskyblue]'
print '    "json_spirit" [color=red]'
print '    "libevmjit" [style=filled,fillcolor=LavenderBlush]'
print '    "scrypt" [color=red]'
print '    "secp256k1" [color=red]'
print ''
print '    "base" -> "boost"'
print '    "base" -> "json_spirit"'
print '    "base" -> "LevelDB"'
print '    "base" -> "pthreads" [style=dotted]'
print '    "curl" -> "ssh2"  [style=dotted]'
print '    "curl" -> "openssl"  [style=dotted]'
print '    "curl" -> "zlib"  [style=dotted]'
print '    "ethereum" -> "libevmjit" [style=dotted]'
print '    "json-rpc-cpp" -> "curl"'
print '    "json-rpc-cpp" -> "microhttpd"'
print '    "json-rpc-cpp" -> "Jsoncpp"'
print '    "json-rpc-cpp" -> "argtable2" [style=dotted]'
print '    "LevelDB" -> "snappy" [style=dotted]'
print '    "libevmjit" -> "libedit"'
print '    "libevmjit" -> "llvm"'
print '    "secp256k1" -> "gmp"'
print ''

processRepository('../..')

print "}"
