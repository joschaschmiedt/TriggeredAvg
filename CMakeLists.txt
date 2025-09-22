cmake_minimum_required(VERSION 3.15.0)
if (NOT DEFINED GUI_BASE_DIR)
	if (DEFINED ENV{GUI_BASE_DIR})
		set(GUI_BASE_DIR $ENV{GUI_BASE_DIR})
	else()
		set(GUI_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../plugin-GUI)
	endif()
endif()

get_filename_component(PROJECT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(PLUGIN_NAME ${PROJECT_FOLDER} NAME)

project(OE_PLUGIN_${PLUGIN_NAME})
set(CMAKE_SHARED_LIBRARY_PREFIX "")
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(LINUX 1)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Debug)
	endif()
endif()

set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
	OEPLUGIN
	"$<$<PLATFORM_ID:Windows>:JUCE_API=__declspec(dllimport)>"
	$<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
	$<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
	$<$<CONFIG:Debug>:DEBUG=1>
	$<$<CONFIG:Debug>:_DEBUG=1>
	$<$<CONFIG:Release>:NDEBUG=1>
	)


set(SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Source)
file(GLOB_RECURSE SRC_FILES LIST_DIRECTORIES false "${SOURCE_PATH}/*.cpp" "${SOURCE_PATH}/*.h")
set(GUI_COMMONLIB_DIR ${GUI_BASE_DIR}/installed_libs)

set(CONFIGURATION_FOLDER $<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>)

list(APPEND CMAKE_PREFIX_PATH ${GUI_COMMONLIB_DIR} ${GUI_COMMONLIB_DIR}/${CONFIGURATION_FOLDER})

if (APPLE)
	add_library(${PLUGIN_NAME} MODULE ${SRC_FILES})
else()
	add_library(${PLUGIN_NAME} SHARED ${SRC_FILES})
endif()

target_compile_features(${PLUGIN_NAME} PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_20)
target_include_directories(${PLUGIN_NAME} PUBLIC ${GUI_BASE_DIR}/JuceLibraryCode ${GUI_BASE_DIR}/JuceLibraryCode/modules ${GUI_BASE_DIR}/Plugins/Headers ${GUI_COMMONLIB_DIR}/include)

set(GUI_BIN_DIR ${GUI_BASE_DIR}/Build/${CONFIGURATION_FOLDER})

if (NOT CMAKE_LIBRARY_ARCHITECTURE)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(CMAKE_LIBRARY_ARCHITECTURE "x64")
	else()
		set(CMAKE_LIBRARY_ARCHITECTURE "x86")
	endif()
endif()

#Libraries and compiler options
if(MSVC)
	target_link_libraries(${PLUGIN_NAME} ${GUI_BIN_DIR}/open-ephys.lib)
	target_compile_options(${PLUGIN_NAME} PRIVATE /sdl /W4 /MP /external:W0 /external:anglebrackets)
	
	install(TARGETS ${PLUGIN_NAME} RUNTIME DESTINATION ${GUI_BIN_DIR}/plugins  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})

	set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../libs)
elseif(LINUX)
	target_link_libraries(${PLUGIN_NAME} GL X11 Xext Xinerama asound dl freetype pthread rt)
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

#create filters for vs and xcode

foreach( src_file IN ITEMS ${SRC_FILES})
	get_filename_component(src_path "${src_file}" PATH)
	file(RELATIVE_PATH src_path_rel "${SOURCE_PATH}" "${src_path}")
	string(REPLACE "/" "\\" group_name "${src_path_rel}")
	source_group("${group_name}" FILES "${src_file}")
endforeach()

#additional libraries, if needed
#find_package(LIBNAME)
#or
#find_library(LIBNAME_LIBRARIES NAMES libname)
#find_path(LIBNAME_INCLUDE_DIRS includefile.h)
#
#target_link_libraries(${PLUGIN_NAME} ${LIBNAME_LIBRARIES})

# Testing configuration
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    
    # Create a minimal static library for testing (only include what we need for MultiChannelRingBuffer)
    set(TEST_LIB_NAME ${PLUGIN_NAME}_test_lib)
    set(MINIMAL_SOURCE_FILES 
        ${SOURCE_PATH}/MultiChannelRingBuffer.cpp
        ${SOURCE_PATH}/MultiChannelRingBuffer.h
    )
    add_library(${TEST_LIB_NAME} STATIC ${MINIMAL_SOURCE_FILES})
    
    # Configure the test library with same settings as main plugin
    target_compile_features(${TEST_LIB_NAME} PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_20)
    target_include_directories(${TEST_LIB_NAME} PUBLIC 
        ${SOURCE_PATH}
        ${GUI_BASE_DIR}/JuceLibraryCode 
        ${GUI_BASE_DIR}/JuceLibraryCode/modules 
        ${GUI_BASE_DIR}/Plugins/Headers 
        ${GUI_COMMONLIB_DIR}/include
    )
    target_compile_definitions(${TEST_LIB_NAME} PRIVATE
        OEPLUGIN
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
        $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
    )
    
    # Platform-specific linking for test library
    if(MSVC)
        target_link_libraries(${TEST_LIB_NAME} ${GUI_BIN_DIR}/open-ephys.lib)
        target_compile_options(${TEST_LIB_NAME} PRIVATE /sdl /W4 /MP /external:W0 /external:anglebrackets)
    elseif(LINUX)
        target_link_libraries(${TEST_LIB_NAME} GL X11 Xext Xinerama asound dl freetype pthread rt)
        target_compile_options(${TEST_LIB_NAME} PRIVATE -fPIC -rdynamic -O3)
    elseif(APPLE)
        set_property(TARGET ${TEST_LIB_NAME} APPEND_STRING PROPERTY LINK_FLAGS
        "-undefined dynamic_lookup")
    endif()
    
    # Create test executable
    set(TEST_NAME ${PLUGIN_NAME}_tests)
    file(GLOB_RECURSE TEST_FILES LIST_DIRECTORIES false "Test/*.cpp" "Test/*.h")
    
    add_executable(${TEST_NAME} ${TEST_FILES})
    
    # Link test executable with the static test library and Google Test
    target_link_libraries(${TEST_NAME} 
        ${TEST_LIB_NAME}  # Link to the static test library
        gtest_main
        gmock_main
    )
    
    # Include directories for tests
    target_include_directories(${TEST_NAME} PRIVATE 
        ${SOURCE_PATH}
        ${GUI_BASE_DIR}/JuceLibraryCode 
        ${GUI_BASE_DIR}/JuceLibraryCode/modules 
        ${GUI_BASE_DIR}/Plugins/Headers 
        ${GUI_COMMONLIB_DIR}/include
    )
    
    # Compiler features and definitions for tests
    target_compile_features(${TEST_NAME} PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_20)
    target_compile_definitions(${TEST_NAME} PRIVATE
        OEPLUGIN
        $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
        $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
        $<$<CONFIG:Debug>:DEBUG=1>
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:Release>:NDEBUG=1>
    )
    
    # Platform-specific linking for tests (same as test library)
    if(MSVC)
        target_link_libraries(${TEST_NAME} ${GUI_BIN_DIR}/open-ephys.lib)
        target_compile_options(${TEST_NAME} PRIVATE /sdl /W4 /MP /external:W0 /external:anglebrackets)
    elseif(LINUX)
        target_link_libraries(${TEST_NAME} GL X11 Xext Xinerama asound dl freetype pthread rt)
        target_compile_options(${TEST_NAME} PRIVATE -fPIC -rdynamic -O3)
    elseif(APPLE)
        set_property(TARGET ${TEST_NAME} APPEND_STRING PROPERTY LINK_FLAGS
        "-undefined dynamic_lookup")
    endif()
    
    include(GoogleTest)
    # gtest_discover_tests(${TEST_NAME})  # Commented out due to DLL dependency issues
    
    # Manual test registration for CTest compatibility
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        ENVIRONMENT "PATH=${GUI_BIN_DIR};$ENV{PATH}"
    )
endif()
#target_include_directories(${PLUGIN_NAME} PRIVATE ${LIBNAME_INCLUDE_DIRS})
