# Triggered LFP Viewer

A plugin for the Open Ephys GUI that displays continuous signals triggered by TTL events and/or messages, similar to an oscilloscope triggered display.

## Features

- **Multi-trigger Support**: Responds to TTL lines, broadcast messages, or combined TTL+message triggers
- **Flexible Time Windows**: Configurable pre-trigger (-500ms to -10ms) and post-trigger (10ms to 5000ms) windows
- **Channel Selection**: Display all channels or select specific channels for visualization
- **Multiple Display Modes**: Individual traces, averaged traces, overlay mode, or both
- **Grid Layout**: Up to 4x6 grid of plots for multiple conditions/channels
- **Real-time Updates**: Immediate display updates when triggered data windows are complete
- **Signal Averaging**: Automatic averaging across trials with configurable trial history
- **Oscilloscope-like Interface**: Familiar controls for amplitude and time scaling

## Installation

This plugin can be added via the Open Ephys GUI Plugin Installer. To access the Plugin Installer, press **ctrl-P** or **⌘P** from inside the GUI. Once the installer is loaded, browse to the "Triggered LFP Viewer" plugin and click "Install."

## Usage

### Basic Setup

1. **Add Plugin**: Drag the "Triggered LFP Viewer" from the processor list into your signal chain
2. **Configure Time Windows**: Set pre-trigger and post-trigger windows (default: -500ms to +2000ms)
3. **Select Channels**: Choose which channels to display (default: all channels)
4. **Add Trigger Sources**: Click "Add Trigger" to create trigger conditions

### Trigger Configuration

- **TTL Triggers**: Respond to rising edges on specified TTL lines
- **Message Triggers**: Respond to broadcast messages with matching names
- **Combined Triggers**: Require both TTL edge AND subsequent message

### Display Options

- **Grid Size**: 1x1 up to 4x6 grid layout
- **Display Modes**:
  - Individual: Show each trial separately
  - Average: Show averaged traces across trials
  - Overlay: Overlay multiple conditions
  - Both: Show both individual and averaged traces
- **Scaling**: Adjustable amplitude and time scaling
- **Auto-scale**: Automatic amplitude scaling to data range

### Controls

- **Clear Data**: Reset all collected data
- **Save**: Export traces and statistics
- **Auto Scale**: Automatically adjust amplitude range
- **Open Canvas**: Open the visualization window

Information on the Open Ephys Plugin API can be found on [the GUI's documentation site](https://open-ephys.github.io/gui-docs/Developer-Guide/Open-Ephys-Plugin-API.html).

## Creating a new Visualizer Plugin

1. Click "Use this template" to instantiate a new repository under your GitHub account. 
2. Clone the new repository into a directory at the same level as the `plugin-GUI` repository. This is typically named `OEPlugins`, but it can have any name you'd like.
3. Modify the [OpenEphysLib.cpp file](https://open-ephys.github.io/gui-docs/Developer-Guide/Creating-a-new-plugin.html) to include your plugin's name and version number.
4. Create the plugin [build files](https://open-ephys.github.io/gui-docs/Developer-Guide/Compiling-plugins.html) using CMake.
5. Use Visual Studio (Windows), Xcode (macOS), or `make` (Linux) to compile the plugin.
6. Edit the code to add custom functionality, and add additional source files as needed.

## Repository structure

This repository contains 3 top-level directories:

- `Build` - Plugin build files will be auto-generated here. These files will be ignored in all `git` commits.
- `Source` - All plugin source files (`.h` and `.cpp`) should live here. There can be as many source code sub-directories as needed.
- `Resources` - This is where you should store any non-source-code files, such as library files or scripts.

## Using external libraries

To link the plugin to external libraries, it is necessary to manually edit the Build/CMakeLists.txt file. The code for linking libraries is located in comments at the end.
For most common libraries, the `find_package` option is recommended. An example would be

```cmake
find_package(ZLIB)
target_link_libraries(${PLUGIN_NAME} ${ZLIB_LIBRARIES})
target_include_directories(${PLUGIN_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
```

If there is no standard package finder for cmake, `find_library`and `find_path` can be used to find the library and include files respectively. The commands will search in a variety of standard locations For example

```cmake
find_library(ZMQ_LIBRARIES NAMES libzmq-v120-mt-4_0_4 zmq zmq-v120-mt-4_0_4) #the different names after names are not a list of libraries to include, but a list of possible names the library might have, useful for multiple architectures. find_library will return the first library found that matches any of the names
find_path(ZMQ_INCLUDE_DIRS zmq.h)

target_link_libraries(${PLUGIN_NAME} ${ZMQ_LIBRARIES})
target_include_directories(${PLUGIN_NAME} PRIVATE ${ZMQ_INCLUDE_DIRS})
```

### Providing libraries for Windows

Since Windows does not have standardized paths for libraries, as Linux and macOS do, it is sometimes useful to pack the appropriate Windows version of the required libraries alongside the plugin.
To do so, a _libs_ directory has to be created **at the top level** of the repository, alongside this README file, and files from all required libraries placed there. The required folder structure is:

```
    libs
    ├─ include           #library headers
    ├─ lib
        ├─ x64           #64-bit compile-time (.lib) files
        └─ x86           #32-bit compile time (.lib) files, if needed
    └─ bin
        ├─ x64           #64-bit runtime (.dll) files
        └─ x86           #32-bit runtime (.dll) files, if needed
```

DLLs in the bin directories will be copied to the open-ephys GUI _shared_ folder when installing.
