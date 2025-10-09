# Testing Setup for Triggered-Avg Plugin

This directory contains the testing infrastructure for the triggered-avg plugin
using the Open Ephys main project's testing framework.

Instead of trying to recreate all the complex Open Ephys dependencies, this
approach builds your plugin tests as part of the main Open Ephys project, giving
you access to:

- `gui_testable_source` - The main GUI as a shared library with all JUCE and Open Ephys types
- `test_helpers` - Open Ephys testing utilities
- All Open Ephys types like `TTLEventPtr`, `ContinuousChannel`, etc.

## Prerequisites

1. **Open Ephys GUI source code** at `../../plugin-GUI` (relative to your plugin directory)
2. **Visual Studio 2022** (or compatible compiler)
3. **CMake 3.23+**

## Build and run the tests

- Option 1: Use the build script to build the main gui, the plugin and run the tests
  ```
  build-and-test.ps1
  ```
- Option 2: Manual Build
  ```
  cmake -B build -S . -DBUILD_TESTS=ON
  cmake --build build --config Debug
  ctest --config Debug --output-on-failure
  ```

## Adding New Tests

1. Create new test files in the `../Tests/` directory (e.g., `test_YourNewClass.cpp`)

2. Add the test file to `CMakeLists.txt`:

   ```cmake
   target_sources(triggered-avg-tests PRIVATE
       # ... existing files ...
       ${PLUGIN_DIR}/Tests/test_YourNewClass.cpp
   )
   ```

## Writing Tests with Open Ephys Types

You now have full access to Open Ephys types in your tests:

```cpp
#include "DataCollector.h"
#include <JuceHeader.h>
#include <ProcessorHeaders.h>  // Open Ephys headers
#include <gtest/gtest.h>

TEST(MyTest, WithOpenEphysTypes) {
    // You can create and use real Open Ephys objects
    auto eventChannel = new EventChannel(EventChannel::TTL, 0, 0, 0, "Test");
    auto ttlEvent = TTLEvent::createTTLEvent(eventChannel, nullptr, 1000, 1, true);

    // Test your plugin code with real Open Ephys types
    // ...
}
```

## Important Notes

### Memory Management

- JUCE objects need proper initialization - use `ScopedJuceInitialiser_GUI` in test fixtures
- Clean up Open Ephys objects properly (many use JUCE's reference counting)

### Thread Safety

- If testing threaded components (like `DataCollector`), always stop threads in test teardown
- Use appropriate timeouts when stopping threads

### Platform Differences

- The build is configured for Windows with Visual Studio
- For Linux/Mac, you may need to adjust compiler flags in `CMakeLists.txt`

## Troubleshooting

### "Main GUI directory not found"

- Ensure the Open Ephys GUI source is at `../../../plugin-GUI` relative to this directory
- Or set the path correctly in `CMakeLists.txt`

### Linker Errors

- Make sure you're building with the same configuration (Debug/Release) throughout
- Verify that all source files your tests depend on are listed in `CMakeLists.txt`

### Test Failures

- Use `--output-on-failure` with ctest to see detailed error messages
- Run the test executable directly for more detailed output
- Use a debugger by opening the generated `.sln` file in Visual Studio
