#
# this function requires the following variables to be specified:
# ETH_VERSION
# PROJECT_NAME
# PROJECT_VERSION
# PROJECT_COPYRIGHT_YEAR
# PROJECT_VENDOR
# PROJECT_DOMAIN_SECOND
# PROJECT_DOMAIN_FIRST
# SRC_LIST
# HEADERS
#
# params:
# ICON
#

macro(eth_add_executable EXECUTABLE)
	set (extra_macro_args ${ARGN})
	set (options)
	set (one_value_args ICON)
	set (multi_value_args UI_RESOURCES WIN_RESOURCES)
	cmake_parse_arguments (ETH_ADD_EXECUTABLE "${options}" "${one_value_args}" "${multi_value_args}" "${extra_macro_args}")

	if (APPLE)

		add_executable(${EXECUTABLE} MACOSX_BUNDLE ${SRC_LIST} ${HEADERS} ${ETH_ADD_EXECUTABLE_UI_RESOURCES})
		set(PROJECT_VERSION "${ETH_VERSION}")
		set(MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
		set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_NAME} ${PROJECT_VERSION}")
		set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
		set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
		set(MACOSX_BUNDLE_COPYRIGHT "${PROJECT_COPYRIGHT_YEAR} ${PROJECT_VENDOR}")
		set(MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_DOMAIN_SECOND}.${PROJECT_DOMAIN_FIRST}")
		set(MACOSX_BUNDLE_BUNDLE_NAME ${EXECUTABLE})
		set(MACOSX_BUNDLE_ICON_FILE ${ETH_ADD_EXECUTABLE_ICON})
		set_target_properties(${EXECUTABLE} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/EthereumMacOSXBundleInfo.plist.in")
		set_source_files_properties(${EXECUTABLE} PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
		set_source_files_properties(${MACOSX_BUNDLE_ICON_FILE}.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	else ()
		add_executable(${EXECUTABLE} ${ETH_ADD_EXECUTABLE_UI_RESOURCES}  ${ETH_ADD_EXECUTABLE_WIN_RESOURCES} ${SRC_LIST} ${HEADERS})
	endif()

endmacro()

macro(eth_copy_dll EXECUTABLE DLL)
	# dlls must be unsubstitud list variable (without ${}) in format
	# optimized;path_to_dll.dll;debug;path_to_dlld.dll 
	if(DEFINED MSVC)
		list(GET ${DLL} 1 DLL_RELEASE)
		list(GET ${DLL} 3 DLL_DEBUG)
		add_custom_command(TARGET ${EXECUTABLE}
			PRE_BUILD 
			COMMAND ${CMAKE_COMMAND} ARGS 
			-DDLL_RELEASE="${DLL_RELEASE}" 
			-DDLL_DEBUG="${DLL_DEBUG}" 
			-DCONF="$<CONFIGURATION>"
			-DDESTINATION="${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}" 
			-P "${ETH_SCRIPTS_DIR}/copydlls.cmake"
		)
	endif()
endmacro()

macro(eth_copy_dlls EXECUTABLE)
	foreach(dll ${ARGN})
		eth_copy_dll(${EXECUTABLE} ${dll})
	endforeach(dll)
endmacro()

# 
# this function requires the following variables to be specified:
# ETH_DEPENDENCY_INSTALL_DIR
#
# params: 
# QMLDIR
#

macro(eth_install_executable EXECUTABLE)

	set (extra_macro_args ${ARGN})
	set (options)
	set (one_value_args QMLDIR)
	set (multi_value_args DLLS)
	cmake_parse_arguments (ETH_INSTALL_EXECUTABLE "${options}" "${one_value_args}" "${multi_value_args}" "${extra_macro_args}")
	
	if (ETH_INSTALL_EXECUTABLE_QMLDIR)
		if (APPLE)
			set(eth_qml_dir "-qmldir=${ETH_INSTALL_EXECUTABLE_QMLDIR}")
		elseif (WIN32)
			set(eth_qml_dir "--qmldir ${ETH_INSTALL_EXECUTABLE_QMLDIR}")
		endif()
		message(STATUS "${EXECUTABLE} qmldir: ${eth_qml_dir}")
	endif()

	if (APPLE)
		# First have qt5 install plugins and frameworks
		add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
			COMMAND ${MACDEPLOYQT_APP} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${EXECUTABLE}.app -executable=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${EXECUTABLE}.app/Contents/MacOS/${EXECUTABLE} ${eth_qml_dir}
			WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
			COMMAND sh ${ETH_SCRIPTS_DIR}/macdeployfix.sh ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${EXECUTABLE}.app/Contents
		)
			
		# This tool and next will inspect linked libraries in order to determine which dependencies are required
		if (${CMAKE_CFG_INTDIR} STREQUAL ".")
			# TODO: This should only happen for GUI application
			set(APP_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}.app")
		else ()
			set(APP_BUNDLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/\$ENV{CONFIGURATION}/${EXECUTABLE}.app")
		endif ()

		install(CODE "
			include(BundleUtilities)
			set(BU_CHMOD_BUNDLE_ITEMS 1)
			verify_app(\"${APP_BUNDLE_PATH}\")
			" COMPONENT RUNTIME )

	elseif (DEFINED MSVC)

		get_target_property(TARGET_LIBS ${EXECUTABLE} INTERFACE_LINK_LIBRARIES)
		string(REGEX MATCH "Qt5::Core" HAVE_QT ${TARGET_LIBS})
		if ("${HAVE_QT}" STREQUAL "Qt5::Core")
			add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
				COMMAND cmd /C "set PATH=${Qt5Core_DIR}/../../../bin;%PATH% && ${WINDEPLOYQT_APP} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${EXECUTABLE}.exe ${eth_qml_dir}"
				WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
			)
			#workaround for https://bugreports.qt.io/browse/QTBUG-42083
			add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
				COMMAND cmd /C "(echo [Paths] & echo.Prefix=.)" > "qt.conf"
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR} VERBATIM
			)
		endif()

		#copy additional dlls
		foreach(dll ${ETH_INSTALL_EXECUTABLE_DLLS})
			eth_copy_dll(${EXECUTABLE} ${dll})
		endforeach(dll)

		install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Debug"
			DESTINATION .
			CONFIGURATIONS Debug
			COMPONENT ${EXECUTABLE}
		)

		install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Release"
			DESTINATION .
			CONFIGURATIONS Release
			COMPONENT ${EXECUTABLE}
		)

		install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/"
			DESTINATION bin
			CONFIGURATIONS RelWithDebInfo
			COMPONENT ${EXECUTABLE}
		)

	else()
		install( TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
	endif ()

endmacro()

macro (eth_name KEY VALUE)
	if (NOT (APPLE OR WIN32))
		string(TOLOWER ${VALUE} LVALUE )
		set(${KEY} ${LVALUE})
	else()
		set(${KEY} ${VALUE})
	endif()
endmacro()

macro(eth_package EXECUTABLE)

	set (extra_macro_args ${ARGN})
	set (options)
	set (one_value_args OSX_ICON WIN_ICON NAME DESCRIPTION)
	set (multi_value_args DLLS)
	cmake_parse_arguments (ETH_INSTALL_EXECUTABLE "${options}" "${one_value_args}" "${multi_value_args}" "${extra_macro_args}")

	if (APPLE)
		add_custom_target(appdmg
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			COMMAND ${CMAKE_COMMAND}
			-DAPP_DMG_EXE=${ETH_APP_DMG}
			-DAPP_DMG_FILE=appdmg.json.in
			-DAPP_DMG_ICON="${ETH_INSTALL_EXECUTABLE_OSX_ICON}"
			-DAPP_DMG_BACKGROUND="install-folder-bg.png"
			-DETH_BUILD_DIR="${CMAKE_BINARY_DIR}"
			-DETH_ALETHZERO_APP="$<TARGET_FILE_DIR:${EXECUTABLE}>"
			-P "${ETH_SCRIPTS_DIR}/appdmg.cmake"
		)
	endif ()

	if (DEFINED MSVC)
		# packaging stuff
		include(InstallRequiredSystemLibraries)
		set(CPACK_PACKAGE_NAME "${NAME}")
		set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${ETH_INSTALL_EXECUTABLE_DESCRIPTION}")
		set(CPACK_PACKAGE_VENDOR "ethereum.org")
		set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
		set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
		set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
		set(CPACK_GENERATOR "NSIS")

		# nsis specific stuff
		if (CMAKE_CL_64)
			set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
			set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION} (Win64)")
		else ()
			set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
			set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
		endif()

		set(CPACK_NSIS_DISPLAY_NAME "${NAME}")
		set(CPACK_NSIS_HELP_LINK "https://github.com/ethereum/cpp-ethereum")
		set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/ethereum/cpp-ethereum")
		set(CPACK_NSIS_CONTACT "ethereum.org")
		set(CPACK_NSIS_MODIFY_PATH ON)
		set(CPACK_NSIS_MUI_ICON "${ETH_INSTALL_EXECUTABLE_WIN_ICON}")
		set(CPACK_NSIS_MUI_UNIICON "${ETH_INSTALL_EXECUTABLE_WIN_ICON}")

		include(CPack)
	endif ()

endmacro()

