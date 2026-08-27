# Triggered Average

> [!IMPORTANT]
> **This repository is archived and no longer maintained.**
>
> It has been superseded by [**brain-bremen/event-triggered-analysis**](https://github.com/brain-bremen/event-triggered-analysis), which contains the Triggered Average plugin along with other event-based analysis plugins. Please use that repository for the latest version, issues, and contributions.

A plugin for the Open Ephys GUI that averages continuous signals triggered by TTL events and/or messages, similar to a triggered display on an oscilloscope. The plugin is still work-in-progress and may contain bugs or unexpected behavior. Please report any issues you encounter. 

![Triggered Average Screenshot](Resources/screenshot_canvas.png)

## Installation

...

## Usage

### Basic Setup

Instructions for basic plugin usage will be documented here.

### Trigger Configuration

The plugin supports multiple trigger sources including TTL events and broadcast messages. Data is captured around these events. Triggers and capture parameters can be configured in the editor window.

![Trigger Configuration](Resources/screenshot_editor.png)

### Display Options

The plugin provides real-time visualization of averaged signals with configurable pre- and post-trigger windows. 

## Building from source

Instructions for building the plugin from source can be found in the [Developer Guide](DEVELOPER_GUIDE.md).


## Attribution

This plugin was developed based on the [Online PSTH plugin](https://github.com/open-ephys-plugins/online-psth) by Joscha Schmiedt for the Brain Research Institute at University of Bremen, licensed under the GPLv3 (see [LICENSE.md](LICENSE.md))
