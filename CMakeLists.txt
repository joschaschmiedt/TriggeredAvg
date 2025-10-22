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

# ensure GUI folder exists
if (NOT EXISTS ${GUI_BASE_DIR})
    message(FATAL_ERROR "GUI_BASE_DIR does not exist: ${GUI_BASE_DIR}")
endif()


get_filename_component(PROJECT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(LINUX 1)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()
endif()

set(TRIGGERED_AVG_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(GUI_COMMONLIB_DIR ${GUI_BASE_DIR}/installed_libs)
set(CONFIGURATION_FOLDER $<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>)
list(APPEND CMAKE_PREFIX_PATH ${GUI_COMMONLIB_DIR} ${GUI_COMMONLIB_DIR}/${CONFIGURATION_FOLDER})
set(GUI_BIN_DIR ${GUI_BASE_DIR}/Build/${CONFIGURATION_FOLDER})
message(DEBUG "Using GUI_BASE_DIR: " ${GUI_BASE_DIR})
message(DEBUG "GUI_BIN_DIR will be: " ${GUI_BASE_DIR}/Build/[Debug|Release])


add_subdirectory(${TRIGGERED_AVG_SOURCE_PATH}) # sets TRIGGERED_AVG_HEADERS and TRIGGERED_AVG_SOURCES
MESSAGE(DEBUG "TRIGGERED_AVG_HEADERS: ${TRIGGERED_AVG_HEADERS}")
MESSAGE(DEBUG "TRIGGERED_AVG_SOURCES: ${TRIGGERED_AVG_SOURCES}")
if (APPLE)
    add_library(${PLUGIN_NAME} MODULE ${TRIGGERED_AVG_SOURCES})
else()
    add_library(${PLUGIN_NAME} SHARED ${TRIGGERED_AVG_SOURCES})
endif()

target_sources(${PLUGIN_NAME}
 PRIVATE
  FILE_SET HEADERS
  BASE_DIRS ${TRIGGERED_AVG_SOURCE_PATH}
  FILES
   ${TRIGGERED_AVG_HEADERS}
 PRIVATE
    ${TRIGGERED_AVG_SOURCES}
)

# Organize files into folders in Visual Studio Solution Explorer
source_group(TREE ${TRIGGERED_AVG_SOURCE_PATH}
             PREFIX "Source Files"
             FILES ${TRIGGERED_AVG_SOURCES})
source_group(TREE ${TRIGGERED_AVG_SOURCE_PATH}
             PREFIX "Header Files"
             FILES ${TRIGGERED_AVG_HEADERS})

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
 )

    target_compile_options(${PLUGIN_NAME} PRIVATE /sdl /W3 /MP /external:W0
                           /external:anglebrackets)

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
