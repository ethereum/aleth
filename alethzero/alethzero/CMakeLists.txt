set(CMAKE_INCLUDE_CURRENT_DIR ON)
file(GLOB_RECURSE SRC_LIST "*.cpp" "plugins/*.cpp")
file(GLOB_RECURSE HEADERS "*.h" "plugins/*.h")
file(GLOB_RECURSE UI_RESOURCES "*.ui" "plugins/*.ui")

# required for eth_add_resources, TODO: remove this
if (DEFINED ethereum_SOURCE_DIR)
	set(ETH_DIR ${ethereum_SOURCE_DIR})
else()
	set(ETH_DIR "${CMAKE_CURRENT_LIST_DIR}/../cpp-ethereum"         CACHE PATH "The path to the cpp-ethereum directory")
endif()

eth_name(EXECUTABLE AlethZero)

# eth_add_executable is defined in cmake/EthExecutableHelper.cmake
eth_add_executable(${EXECUTABLE}
	ICON alethzero
	UI_RESOURCES ${UI_RESOURCES} alethzero.icns
	WIN_RESOURCES alethzero.rc
)

target_link_libraries(${EXECUTABLE} aleth)
target_include_directories(${EXECUTABLE} PUBLIC ..)
eth_use(${EXECUTABLE} REQUIRED Qt::Core Qt::Widgets Eth::ethereum Web3::web3jsonrpc Eth::ethashseal Solidity::lll)
if (SOLIDITY)
	eth_use(${EXECUTABLE} REQUIRED Solidity::solidity)
endif()

# required because AUTOUIC can't handle multiple ui includes in one cpp file
qt5_wrap_ui(ui_AlethZero.h AlethZero.ui)
qt5_wrap_ui(ui_Connect.h Connect.ui)

eth_install_executable(${EXECUTABLE})