
if (NOT APP_DMG_EXE)
	message(FATAL_ERROR "Please install appdmg! https://github.com/LinusU/node-appdmg")
endif()

string(REPLACE "/Contents/MacOS" "" ETH_MIX_APP "${ETH_MIX_APP}")
string(REPLACE "/Contents/MacOS" "" ETH_ALETHZERO_APP "${ETH_ALETHZERO_APP}")

set(OUTFILE "${ETH_BUILD_DIR}/appdmg.json")

configure_file(${APP_DMG_FILE} ${OUTFILE})

execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${APP_DMG_ICON}" "${ETH_BUILD_DIR}/appdmg_icon.icns")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy "${APP_DMG_BACKGROUND}" "${ETH_BUILD_DIR}/appdmg_background.png")
execute_process(COMMAND ${CMAKE_COMMAND} -E remove "${ETH_BUILD_DIR}/cpp-ethereum-osx.dmg") 
execute_process(COMMAND ${APP_DMG_EXE} ${OUTFILE} "${ETH_BUILD_DIR}/cpp-ethereum-osx.dmg" RESULT_VARIABLE RETURN_CODE)

if (NOT RETURN_CODE EQUAL 0)
  message(FATAL_ERROR "Failed to run 'appdmg' npm module.  Is it correctly installed?")
endif()
