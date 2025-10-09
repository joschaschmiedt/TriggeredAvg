cmake_minimum_required(VERSION 3.23.0)

# Set up paths
set(MAIN_GUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../plugin-GUI)
set(PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Ensure main GUI directory exists
if(NOT EXISTS ${MAIN_GUI_DIR})
    message(FATAL_ERROR "Main GUI directory not found at: ${MAIN_GUI_DIR}")
endif()

# Build the main project with tests enabled
set(BUILD_TESTS ON CACHE BOOL "Enable testing" FORCE)

# Include the main project - this will create gui_testable_source and test_helpers
add_subdirectory(${MAIN_GUI_DIR} main_project_build)

add_subdirectory(${PLUGIN_DIR}/Source Build) # sets TRIGGERED_AVG_SOURCES and TRIGGERED_AVG_HEADERS

# Now set up our plugin testing project
project(triggered-avg-tests)

# Set up paths for the main project components
set(GUI_BASE_DIR ${MAIN_GUI_DIR})
set(JUCE_DIRECTORY ${MAIN_GUI_DIR}/JuceLibraryCode)

# Find Google Test (reuse what the main project set up)
include(GoogleTest)

# Create our plugin test executable
add_executable(triggered-avg-tests)

MESSAGE(STATUS "SRC ${TRIGGERED_AVG_SOURCES}")

# Add your plugin's source files that need to be tested
target_sources(triggered-avg-tests PRIVATE
    # Core source files from your plugin
    ${TRIGGERED_AVG_SOURCES}
    ${TRIGGERED_AVG_HEADERS}

    # Test files
    ${PLUGIN_DIR}/Tests/test_MultiChannelRingBuffer.cpp
    # Add more test files here as you create them
    # ${PLUGIN_DIR}/Tests/test_DataCollector.cpp
)

# Link against the main project's testable infrastructure
target_link_libraries(triggered-avg-tests
    PRIVATE
        gui_testable_source    # From main project - provides JUCE + Open Ephys types
        test_helpers          # From main project - provides testing utilities
        gtest_main           # Google Test main function
)

# Set up include directories
target_include_directories(triggered-avg-tests PRIVATE
    # Your plugin source directory
    ${PLUGIN_DIR}/Source

    # Main project directories
    ${MAIN_GUI_DIR}/JuceLibraryCode
    ${MAIN_GUI_DIR}/JuceLibraryCode/modules
    ${MAIN_GUI_DIR}/Plugins/Headers
    ${MAIN_GUI_DIR}/Source

    # Test helpers
    ${MAIN_GUI_DIR}/Tests/TestHelpers/include
)

# Set compile definitions - same as main project plugins
target_compile_definitions(triggered-avg-tests PRIVATE
    OEPLUGIN
    BUILD_TESTS
    "$<$<PLATFORM_ID:Windows>:JUCE_API=__declspec(dllimport)>"
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
    $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>

    # Additional definitions that might be needed for testing
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
)

# Set C++ standard
target_compile_features(triggered-avg-tests PRIVATE cxx_std_17)

# Platform-specific compiler options (match main project)
if(MSVC)
    target_compile_options(triggered-avg-tests PRIVATE /sdl- /W0)
elseif(UNIX AND NOT APPLE)
    target_compile_options(triggered-avg-tests PRIVATE -fPIC -rdynamic -O3)
endif()

# Set the output directory to match the main project's test structure
set_target_properties(triggered-avg-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/TestBin/triggered-avg)

# Copy DLLs from common directory to our test directory after build
add_custom_command(TARGET triggered-avg-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/TestBin/common
    ${CMAKE_BINARY_DIR}/TestBin/triggered-avg)

# Ensure DLLs are built before our tests
add_dependencies(triggered-avg-tests gui_testable_source test_helpers)

# Set the output directory to match the main project's test structure
set_target_properties(triggered-avg-tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/TestBin/triggered-avg)

# Copy DLLs from common directory to our test directory after build
add_custom_command(TARGET triggered-avg-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/TestBin/common
    ${CMAKE_BINARY_DIR}/TestBin/triggered-avg)

# Add tests to CTest so they can be discovered and run
gtest_discover_tests(triggered-avg-tests)

# Print some helpful information
message(STATUS "Triggered Avg Tests Configuration:")
message(STATUS "  Main GUI Directory: ${MAIN_GUI_DIR}")
message(STATUS "  Plugin Directory: ${PLUGIN_DIR}")
message(STATUS "  Build Tests: ${BUILD_TESTS}")
