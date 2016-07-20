#!/usr/bin/env python
#
# generate.py
#
# Python script to generate a DOT graph showing the dependency graph of
# the components within the Ethereum webthree-umbrella project.
#
# See http://github.com/ethereum/webthree-umbrella for more info on webthree
# See http://ethereum.org for more info on Ethereum
#
# Contributed by Bob Summerwill (bob@summerwill.net)

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
                            toNode = "JsonRpc"
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
                if not subDirectoryName.startswith("lib"):
                    outputString = outputString + \
                                    ' [shape="box", style="bold"]'
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

    print "    subgraph cluster_cppethereum {"
    print '        label = <https://github.com/ethereum/cpp-ethereum>'
    print "        bgcolor = LavenderBlush"
    print '        "buildinfo"'
    print '        "base"'
    print '        "json_spirit"'
    print '        "scrypt"'
    print '        "secp256k1"'
    
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

    print "    }"


print 'digraph webthree {'
print '    graph [ label   = "Ethereum C++ dependencies" ]'
print '    node  [ fontname = "Courier", fontsize = 10 ]'
print ''
print '    compound = true'

# Hard-coded cluster for webthree-helpers, which does contain any
# of the Ethereum libraries or executables, but does define CMake
# rules which introduce implicit dependencies.   Parsing those would
# be way too much work.   Easier to hard-code them.   This script is
# not attempting to be a general CMake-dependencies graph generator,
# after all.   It's specific to webthree-umbrella.

processRepository('../..')

print "    subgraph cluster_external {"
print '        label = <https://github.com/ethereum/cpp-dependencies>'
print "        bgcolor = HoneyDew"
print '        "boost"'
print '        "buildinfo"'
print '        "base"'
print '        "gmp"'
print '        "Jsoncpp"'
print '        "json_spirit"'
print '        "LevelDB"'
print '        "pthreads"'
print '        "scrypt"'
print '        "secp256k1"'
print "    }"
print '    "base" -> "boost"'
print '    "base" -> "Jsoncpp"'
print '    "base" -> "json_spirit"'
print '    "base" -> "LevelDB"'
print '    "base" -> "pthreads"'
print '    "secp256k1" -> "gmp"'

print "}"
