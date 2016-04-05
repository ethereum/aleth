set(CMAKE_INCLUDE_CURRENT_DIR ON)

aux_source_directory(. SRC_LIST)

file(GLOB HEADERS "*.h")
file(GLOB UI_RESOURCES "*.ui")

# This is a hack until someone proposes a better solution. When building split repos
# ETH_DIR has not been set yet but is nonetheless used in the eth_add_resources() call.
# TODO: fix properly
set(ETH_DIR "${ETH_CMAKE_DIR}/../../libethereum" CACHE PATH "The path to the ethereum directory")
eth_add_resources("${CMAKE_CURRENT_SOURCE_DIR}/JSResources.cmake" "JSRES" "${ETH_DIR}/..")

set(EXECUTABLE aleth)

# it should be static library, cause there is a problem with creating QWidgets from dynamic libraries
# it fails with "Must construct a QApplication before a QWidget"
# https://forum.qt.io/topic/39042/can-t-create-a-qwidget-from-dynamic-library-must-construct-a-qapplication-before-a-qwidget
add_library(${EXECUTABLE} STATIC ${SRC_LIST} ${JSRES} ${HEADERS} ${UI_RESOURCES})

set_target_properties(${EXECUTABLE} PROPERTIES AUTOUIC ON AUTOMOC ON)
eth_use(${EXECUTABLE} REQUIRED Qt::Core Qt::Widgets Qt::Network Web3::webthree Eth::ethcore Eth::p2p Eth::devcrypto Eth::ethereum Web3::web3jsonrpc Eth::natspec Eth::ethashseal)
