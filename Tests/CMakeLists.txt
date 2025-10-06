cmake_minimum_required(VERSION 3.23.0)

# Enable testing
enable_testing()

# Find or fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Create test executable with only necessary sources
add_executable(triggered-avg-tests)
target_sources(triggered-avg-tests 
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/test_MultiChannelRingBuffer.cpp)

# Link against Google Test
target_link_libraries(triggered-avg-tests
    PRIVATE
		triggered-avg-core
		GTest::gtest_main
)

# Set up include directories - use the same as the main plugin
target_include_directories(triggered-avg-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source
    ${GUI_BASE_DIR}/JuceLibraryCode
    ${GUI_BASE_DIR}/JuceLibraryCode/modules
    ${GUI_BASE_DIR}/Plugins/Headers
    ${GUI_COMMONLIB_DIR}/include
)

# Set compile features
target_compile_features(triggered-avg-tests PUBLIC cxx_auto_type cxx_generalized_initializers cxx_std_20)

# Set compile definitions - same as main plugin but for testing
target_compile_definitions(triggered-avg-tests PRIVATE
    OEPLUGIN
    "$<$<PLATFORM_ID:Windows>:JUCE_API=__declspec(dllimport)>"
    $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>
    $<$<PLATFORM_ID:Linux>:JUCE_DISABLE_NATIVE_FILECHOOSERS=1>
    $<$<CONFIG:Debug>:DEBUG=1>
    $<$<CONFIG:Debug>:_DEBUG=1>
    $<$<CONFIG:Release>:NDEBUG=1>
    # Disable some JUCE warnings for tests
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
        JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_SSE_INTRINSICS=1  # if on Intel
)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(triggered-avg-tests)

