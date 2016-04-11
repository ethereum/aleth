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
                        # Merge all Qt::* and JsonRpc::* nodes
                        # to simplify the output graph.   Not much
                        # value in the details there.
                        if toNode.startswith("Qt::"):
                            toNode = "Qt*"
                        elif toNode.startswith("JsonRpc::"):
                            toNode = "JsonRpc*"
                        elif "::" in toNode:
                            toNode = toNode.split("::")[1]
                        edgeText = '"' + fromNode + '" -> "' + toNode + '"'
                        if "OPTIONAL" in line:
                            edgeText = edgeText + " [style=dotted]"
                        if (fromNode == "web3jsonrpc" and toNode == "solidity"):
                            edgeText = "" # do nothing
                        elif (fromNode == "aleth" and toNode == "natspec"):
                            edgeText = "" # do nothing
                        elif (fromNode == "solidity" and toNode == "evmasm"):
                            edgeText = "" # do nothing
                        elif (fromNode == "solidity" and toNode == "evmcore"):
                            edgeText = "" # do nothing
                        elif (fromNode == "solidity" and toNode == "devcore"):
                            edgeText = "" # do nothing
                        elif (fromNode == "evmasm"):
                            edgeText = "" # do nothing
                        elif (toNode == "evmasm"):
                            edgeText = "" # do nothing
                        elif (fromNode == "lllc"):
                            edgeText = "" # do nothing
                        elif (fromNode == "lll"):
                            edgeText = "" # do nothing
                        elif (toNode == "lll"):
                            edgeText = "" # do nothing
                        elif (toNode == "jsconsole"):
                            edgeText = "" # do nothing
                        elif (fromNode == "jsconsole"):
                            edgeText = "" # do nothing
                        elif (toNode == "jsengine"):
                            edgeText = "" # do nothing
                        elif (toNode == "natspec"):
                            edgeText = "" # do nothing
                        elif (toNode == "V8"):
                            edgeText = "" # do nothing
                        elif (toNode == "Readline"):
                            edgeText = "" # do nothing
                        else:
                            outputString = outputString + edgeText + "\n"

    return outputString


# Return a string which is a list of all the library and application
# names within a given git sub-module directory.
def getLibraryAndApplicationNames(submodulePath):
    outputString = ""
    for subDirectoryName in os.listdir(submodulePath):
        absSubDirectoryPath = os.path.join(submodulePath, subDirectoryName)
        if os.path.isdir(absSubDirectoryPath):
            #print "BALAHAHAHA - " + subDirectoryName
            if (subDirectoryName == "liblll"):
                continue
            elif (subDirectoryName == "lllc"):
                continue
            elif (subDirectoryName == "libevmasm"):
                continue
            elif (subDirectoryName == "libnatspec"):
                continue
            elif (subDirectoryName == "libevmcore"):
                continue
            elif (subDirectoryName == "libjsconsole"):
                continue
            elif (subDirectoryName == "libjsengine"):
                continue
            elif (subDirectoryName == "V8"):
                continue
            elif (subDirectoryName == "Readline"):
                continue
            else:
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
def processSubmodule(root, submodule):
    submodulePath = os.path.join(root, submodule)
    cleanName = submodule.replace(".", "_").replace("-", "_")

    if (submodule == "libethereum"):
        cleanName = "cpp_ethereum"
        submodule = "cpp_ethereum" 
    elif (submodule == "libweb3core"):
        cleanName = "cpp_ethereum"
        submodule = "cpp_ethereum" 
    elif (submodule == "webthree"):
        cleanName = "cpp_ethereum"
        submodule = "cpp_ethereum"

    print "    subgraph cluster_" + cleanName + " {"
    print "        label = <" + submodule + " dependencies>"

    # Hard-coded dependency edge for "soljson" which is within
    # an EMSCRIPTEN conditional.   It's not worth bothering with
    # complicating the parsing to cope with conditionals for this
    # single misplaced dependency edge.   We'll just hard-code it.
    if "solidity" in submodule:
        print '    "soljson" -> "solidity"'

    # Mix doesn't have the same sub-module structure as everything else
    if (submodule == "mix"):
        print 'Mix\n'
    else:
        print getLibraryAndApplicationNames(submodulePath)

    if (submodule == "cpp_ethereum"):
        print "        bgcolor = AliceBlue"
    elif (submodule == "libethereum"):
        print "        bgcolor = LavenderBlush"
    elif (submodule == "webthree"):
        print "        bgcolor = Honeydew"
    elif (submodule == "libweb3core"):
        print "        bgcolor = AliceBlue"
    elif (submodule == "mix"):
        print "        bgcolor = Honeydew"
    elif (submodule == "solidity"):
        print "        bgcolor = WhiteSmoke"
    else:
        print "        bgcolor = LightGray"
    print "    }"

    for library in os.listdir(submodulePath):
        if (library == "lll"):
            continue
        elif (library == "lllc"):
            continue
        elif (library == "evmasm"):
            continue
        elif (library == "evmcore"):
            continue
        elif (library == "jsconsole"):
            continue
        elif (library == "jsengine"):
            continue
        elif (library == "libnatspec"):
            continue
        elif (library == "V8"):
            continue
        elif (library == "Readline"):
            continue

        absLibPath = os.path.join(submodulePath, library)
        if os.path.isdir(absLibPath):
            print getDependencyEdges(submodulePath, library)


# Walk the sub-modules under the umbrella
def processUmbrella(root):
    for submodule in os.listdir(root):
        absPath = os.path.join(root, submodule)
        if os.path.isdir(absPath):
            if not (".git" in absPath) \
                and not ("dependency_graph" in absPath) \
                    and not ("webthree-helpers" in absPath):
                        processSubmodule(root, submodule)

    # Mix doesn't have the same sub-module structure as everything else
    print getDependencyEdges(root, "mix")


print 'digraph webthree {'
print '    graph [ label   = "webthree dependencies" ]'
print '    node  [ fontname = "Courier", fontsize = 10 ]'
print ''
print '    compound = true'

# Hard-coded cluster for webthree-helpers, which does contain any
# of the Ethereum libraries or executables, but does define CMake
# rules which introduce implicit dependencies.   Parsing those would
# be way too much work.   Easier to hard-code them.   This script is
# not attempting to be a general CMake-dependencies graph generator,
# after all.   It's specific to webthree-umbrella.
print "    subgraph cluster_cpp_ethereum {"
print '        label = <cpp_ethereum dependencies>'
print '        bgcolor = LemonChiffon'
print '        "buildinfo"'
print '        "base"'
print '        "json_spirit"'
print '        "scrypt"'
print '        "secp256k1"'
print '        "testeth" [shape="box", style="bold"]'
print "    }"
print '    "base" -> "boost"'
print '    "base" -> "Jsoncpp"'
print '    "base" -> "json_spirit"'
print '    "base" -> "LevelDB"'
print '    "base" -> "pthreads"'
print '    "secp256k1" -> "gmp"'
print "    subgraph cluster_cpp_ethereum {"
print '        label = <cpp_ethereum dependencies>'
print '        bgcolor = LemonChiffon'
print '        "buildinfo"'
print '        "base"'
print '        "json_spirit"'
print '        "scrypt"'
print '        "secp256k1"'
print '        "evmcore"'
print "    }"
print "    subgraph cluster_solidity {"
print '        label = <solidity dependencies>'
print '        "lllc" [shape="box", style="bold"]'
print '        "lllc" -> "lll"'
print '        "evmasm"'
print '        "evmcore_solidity"'
print '        "evmasm" -> "evmcore_solidity"'
print '        "lll" -> "evmasm"'
print '        "solidity" -> "evmasm"'
print "    }"

# Hard-coded dependencies for 'libaleth', which doesn't have a UseAleth.cmake
# to go with it, because the library is only used by the Aleth* applications,
# and is not exposed to other applications.
print '    "AlethZero" -> "aleth"'
print '    "AlethOne" -> "aleth"'

processUmbrella('..')

print "}"
