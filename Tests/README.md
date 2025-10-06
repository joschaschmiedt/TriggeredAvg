# Testing Setup for Triggered Average Plugin

This document describes how to build and run tests for the Triggered Average plugin.

## Prerequisites

- CMake 3.15 or higher
- Visual Studio 2019/2022 (on Windows)
- Open Ephys GUI development environment set up

## Building Tests

1. Configure the project with testing enabled:
```bash
cmake -B build -S . -DBUILD_TESTS=ON
```

2. Build the test executable:
```bash
cmake --build build --config Debug --target triggered-avg_tests
```

## Running Tests

### Option 1: Direct execution
```bash
cd build
Debug\triggered-avg_tests.exe
```

### Option 2: Using CTest (if all dependencies are resolved)
```bash
cd build
ctest --output-on-failure
```

## Test Structure

The tests are organized in `tests/` directory:

- `MultiChannelRingBufferTest.cpp` - Comprehensive tests for the MultiChannelRingBuffer class

## Test Coverage


## Adding New Tests

To add tests for other classes:

1. Create a new test file in the `tests/` directory following the naming pattern `*Test.cpp`
2. Include the necessary headers:
   ```cpp
   #include <gtest/gtest.h>
   #include <gmock/gmock.h>
   #include <memory>
   #include <JuceHeader.h>
   #include "YourClassToTest.h"
   ```
3. CMake will automatically pick up the new test files

## Troubleshooting

### DLL Not Found Errors
If you get errors about missing DLLs when running tests:
1. Ensure the Open Ephys GUI is built in the same configuration (Debug/Release)
2. Check that the GUI_BASE_DIR environment variable is set correctly
3. Copy required DLLs to the test executable directory if needed

### Build Errors
- Verify that all dependencies are properly installed
- Check that the Open Ephys GUI development environment is correctly set up
- Ensure CMake can find all required libraries

## Test Philosophy

These tests focus on:
- **Unit testing**: Testing individual classes in isolation
- **Behavioral testing**: Verifying correct behavior under various conditions
- **Edge case testing**: Ensuring robustness with unusual inputs
- **Thread safety**: Basic verification of concurrent access patterns

The tests use Google Test framework features:
- `TEST_F` for fixture-based tests with common setup/teardown
- `EXPECT_*` macros for non-fatal assertions
- `ASSERT_*` macros for fatal assertions that stop test execution
- Helper functions for creating test data and verifying results