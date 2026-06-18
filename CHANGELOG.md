# Changelog

All notable changes to the TriggeredAvg plugin will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.1] - 2026-06-18

### Fixed

- Null dereference in `handleAsyncUpdate` when the canvas is not open
- Static local `lastSampleNumber` was shared across processor instances, causing incorrect trigger detection with multiple instances
- Data race in cached plot-path rebuild — `DataStore` lock is now held for the full rebuild
- Custom X-axis limits not clamped to the data collection window (`[-pre_ms, post_ms]`), silently producing a blank plot for out-of-range values (#8)
- `x_min`, `x_max`, and `use_custom_x_limits` parameters not handled in `parameterValueChanged`, so programmatic changes (XML load, config messages) had no effect on the display

### Changed

- Replaced `assert(false)` in the `MSG_TRIGGER` broadcast path with a log warning so the plugin degrades gracefully on unexpected message types
- Removed the dead 60 Hz polling timer in `TriggeredAvgCanvas` (display updates are now purely event-driven via `AsyncUpdater`)
- Removed unused `post_ms` member from `GridDisplay`
- Moved `SampleNumber` type alias to a dedicated `Types.h` to remove header coupling

## [0.1.0] - 2026-02-10

### Added

- Initial release of TriggeredAvg plugin
- Event-triggered averaging functionality for continuous data
- Real-time visualization of averaged waveforms
- Configurable pre-trigger and post-trigger windows
- Compatible with Open Ephys GUI API v10
