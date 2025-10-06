cmake_minimum_required(VERSION 3.23.0)

set(PLUGIN_NAME triggered-avg)
project(OE_PLUGIN_${PLUGIN_NAME})

if (NOT DEFINED GUI_BASE_DIR)
	if (DEFINED ENV{GUI_BASE_DIR})
		set(GUI_BASE_DIR $ENV{GUI_BASE_DIR})
	else()
		set(GUI_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin-GUI)
	endif()
endif()

get_filename_component(PROJECT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(LINUX 1)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Debug)
	endif()
endif()

set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(GUI_COMMONLIB_DIR ${GUI_BASE_DIR}/installed_libs)
set(CONFIGURATION_FOLDER $<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>)
list(APPEND CMAKE_PREFIX_PATH ${GUI_COMMONLIB_DIR} ${GUI_COMMONLIB_DIR}/${CONFIGURATION_FOLDER})
set(GUI_BIN_DIR ${GUI_BASE_DIR}/Build/${CONFIGURATION_FOLDER})

message(STATUS "Using GUI_BASE_DIR: " ${GUI_BASE_DIR})
message(STATUS "GUI_BIN_DIR will be: " ${GUI_BASE_DIR}/Build/[Debug|Release])


# STATIC PLUGIN LIBRARY WITH TESTABLE SOURCES
add_library(${PLUGIN_NAME}-core STATIC
		${SOURCE_PATH}/DataCollector.cpp
		${SOURCE_PATH}/GridDisplay.cpp
		${SOURCE_PATH}/MultiChannelRingBuffer.cpp
		${SOURCE_PATH}/PopupConfigurationWindow.cpp
		${SOURCE_PATH}/SinglePlotPanel.cpp
		${SOURCE_PATH}/TimeAxis.cpp
)
target_link_libraries(${PLUGIN_NAME}-core 
	PUBLIC
		${GUI_BIN_DIR}/open-ephys.lib
)

target_compile_definitions(${PLUGIN_NAME}-core 
    PRIVATE
        OEPLUGIN
        "$<$<PLATFORM_ID:Windows>:JUCE_API=__declspec(dllimport)>"
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
        $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
)

target_compile_features(${PLUGIN_NAME}-core PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_20)
target_include_directories(${PLUGIN_NAME}-core
	PUBLIC 
		${GUI_BASE_DIR}/JuceLibraryCode
		${GUI_BASE_DIR}/JuceLibraryCode/modules
		${GUI_BASE_DIR}/Plugins/Headers
		${GUI_COMMONLIB_DIR}/include
)


# SHARED PLUGIN LIBRARY
if (APPLE)
	add_library(${PLUGIN_NAME} MODULE)
else()
	add_library(${PLUGIN_NAME} SHARED)
endif()

target_compile_definitions(${PLUGIN_NAME} 
    PRIVATE
        OEPLUGIN
        "$<$<PLATFORM_ID:Windows>:JUCE_API=__declspec(dllimport)>"
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
        $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
)
target_sources(${PLUGIN_NAME} 
	PRIVATE
		FILE_SET HEADERS 
		BASE_DIRS ${SOURCE_PATH}
		FILES
			${SOURCE_PATH}/TriggeredAvgActions.h
			${SOURCE_PATH}/TriggeredAvgCanvas.h
			${SOURCE_PATH}/TriggeredAvgEditor.h
			${SOURCE_PATH}/TriggeredAvgNode.h
	PRIVATE
		${SOURCE_PATH}/TriggeredAvgActions.cpp
		${SOURCE_PATH}/TriggeredAvgCanvas.cpp
		${SOURCE_PATH}/TriggeredAvgEditor.cpp
		${SOURCE_PATH}/TriggeredAvgNode.cpp
		${SOURCE_PATH}/OpenEphysLib.cpp
)

target_compile_features(${PLUGIN_NAME} PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_20)
target_include_directories(${PLUGIN_NAME} 
	PUBLIC 
		${GUI_BASE_DIR}/JuceLibraryCode
		${GUI_BASE_DIR}/JuceLibraryCode/modules
		${GUI_BASE_DIR}/Plugins/Headers
		${GUI_COMMONLIB_DIR}/include
)


if(MSVC)
	target_link_libraries(${PLUGIN_NAME}
		${GUI_BIN_DIR}/open-ephys.lib
		${PLUGIN_NAME}-core
	)

	target_compile_options(${PLUGIN_NAME} PRIVATE /sdl /W4 /MP /external:W0 /external:anglebrackets)
	
	install(TARGETS ${PLUGIN_NAME} RUNTIME DESTINATION ${GUI_BIN_DIR}/plugins  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})

	set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../libs)
elseif(LINUX)
	target_link_libraries(${PLUGIN_NAME} 
		${PLUGIN_NAME}-core
		GL X11 Xext Xinerama asound dl freetype pthread rt
	)
	set_property(TARGET ${PLUGIN_NAME} APPEND_STRING PROPERTY LINK_FLAGS
		"-fvisibility=hidden -fPIC -rdynamic -Wl,-rpath,'$$ORIGIN/../shared'")
	target_compile_options(${PLUGIN_NAME} PRIVATE -fPIC -rdynamic)
	target_compile_options(${PLUGIN_NAME} PRIVATE -O3) #enable optimization for linux debug
	
	install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION ${GUI_BIN_DIR}/plugins)
elseif(APPLE)
	set_target_properties(${PLUGIN_NAME} PROPERTIES BUNDLE TRUE)
	set_property(TARGET ${PLUGIN_NAME} APPEND_STRING PROPERTY LINK_FLAGS
	"-undefined dynamic_lookup -rpath @loader_path/../../../../shared")

	install(TARGETS ${PLUGIN_NAME} DESTINATION $ENV{HOME}/Library/Application\ Support/open-ephys/plugins-api10)
	set(CMAKE_PREFIX_PATH /opt/local)
endif()


# Testing configuration
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
	add_subdirectory(Tests)
endif()


