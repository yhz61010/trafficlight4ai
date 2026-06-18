# Logging System

This document describes the design, configuration, and per-module instrumentation of trafficlight4ai's logging. Chinese version: [LOGGING_zh.md](LOGGING_zh.md).

## Overview

trafficlight4ai ships a centralized, level-filtered logger that can be toggled, leveled, and inspected (file path) from the Settings dialog. Logs are written to **both a file and the console (stderr)** and rotate by size. The logger is independent of Qt's `qDebug`/`qWarning` (which are stripped by `QT_NO_DEBUG_OUTPUT` in Release builds), so **file logging keeps working in Release builds**.

## Log Levels

Five levels, from most verbose to most severe:

```
VERB < DEBUG < INFO < WARN < ERROR
```

- A configured level records **that level and everything more severe** (e.g. the default `WARN` records only WARN/ERROR).
- **Default level: `WARN`.**
- `VERB` is the most verbose level and logs every breathing-light opacity change (high frequency).

| Level | Use |
|-------|-----|
| `VERB` | High-frequency detail: breathing-light opacity changes, sound-playback cleanup |
| `DEBUG` | Internal flow: widget/tray state transitions, IPC command received, animation start |
| `INFO` | Notable events: state changes, sound playback/settings changes, app startup |
| `WARN` | Recoverable issues: unknown IPC command, invalid sound URL falling back to system beep |
| `ERROR` | Serious failures: IPC listen failure, sound playback failure |

## Log File

- **Path** (fixed; not user-configurable, shown read-only in Settings):
  `<app-data>/logs/trafficlight4ai.log`
  - Linux: `~/.local/share/trafficlight4ai/logs/trafficlight4ai.log`
- **Rotation**: when the file exceeds ~**5 MB** it is renamed to `trafficlight4ai.log.1` (overwriting the previous backup); **one** backup is kept.
- **Line format**:

  ```
  2026-06-18 12:34:56.789 [WARN] [Sound] Invalid sound url for 'x.ogg', falling back to system beep
  ```

  i.e. `timestamp(ms) [LEVEL] [Category] message`.

## Configuration

A new `logging` object is added to `~/.config/trafficlight4ai/config.json`:

```json
"logging": {
  "enabled": true,
  "level": "warn"
}
```

| Field | Default | Description |
|-------|---------|-------------|
| `logging.enabled` | `true` | Write logs (file + console) |
| `logging.level` | `"warn"` | Minimum level: `verb` / `debug` / `info` / `warn` / `error`; invalid values fall back to `warn` |

### Settings Dialog

Three rows are added:

- **Logging**: enable checkbox; disabling it also disables the level combo.
- **Log Level**: `VERB` / `DEBUG` / `INFO` / `WARN` / `ERROR` combo.
- **Log File**: read-only path field + "Open Folder" button.

Changes take effect **immediately** (written to config and applied to the running logger); Cancel reverts them through the snapshot mechanism.

## Architecture

- **`Logger` (`src/Logger.h` / `Logger.cpp`, in `tl4ai_core`)**: singleton.
  - `configure(enabled, level, filePath)` applies the flag/level/path once at startup.
  - `setEnabled()` / `setLevel()` allow live changes from the Settings dialog.
  - `shouldLog(level)` is a **lock-free fast path** (`m_enabled`, `m_level` are `std::atomic`) so high-frequency callers (the breathing animation) can skip building the message string when the level is filtered out.
  - File writes are guarded by a `QMutex`; the `QFile` is kept open and each line is `flush`ed (so logs survive a crash), without `fsync`.
- **Log macros**: `TL_LOGV` / `TL_LOGD` / `TL_LOGI` / `TL_LOGW` / `TL_LOGE`, short-circuiting via `shouldLog` before constructing the message.
- **Qt message forwarding**: `main.cpp` installs `qInstallMessageHandler` to route the framework's own `qDebug`/`qWarning` etc. into `Logger` by mapped level (Debug/Info/Warn/Error), sharing the same file/console sink and level filtering. The project's own `TL_LOG*` calls go straight to `Logger` and do **not** pass through the handler, so there is no double-logging.
- **Release behavior**: `Logger` does not use Qt's `qDebug` family, so it is unaffected by `QT_NO_DEBUG_OUTPUT` and still writes the file in Release.

## Instrumentation Map

| Module | Site | Level |
|--------|------|-------|
| `main.cpp` | App startup (incl. log path/level) | INFO |
| `main.cpp` | State-triggered sound (yellow/green) | INFO |
| `StateManager` | State change | INFO |
| `StateManager` | Unknown IPC command | WARN |
| `IpcServer` | Start listening / restart success | INFO |
| `IpcServer` | Listen failure / path occupied by non-socket | ERROR |
| `IpcServer` | Command received | DEBUG |
| `SoundUtils` | Playback entry (path + URL) | INFO |
| `SoundUtils` | Invalid URL, fallback to system beep | WARN |
| `SoundUtils` | QMediaPlayer playback error | ERROR |
| `SoundUtils` | Playback finished, player cleanup | VERB |
| `SettingsDialog` | Sound toggle (enable/disable) | INFO |
| `SettingsDialog` | Sound file chosen (browse) | INFO |
| `SettingsDialog` | Sound preview | INFO (via `SoundUtils`) |
| `TrafficLightWidget` | **Breathing-light opacity change (`setActiveAlpha`)** | **VERB** |
| `TrafficLightWidget` | State change / animation start | DEBUG |
| `TrayIcon` | State change / icon set | DEBUG |

## Tests

- `tests/test_logger.cpp`: level string conversion (invalid → WARN), `shouldLog` filtering, file writes when enabled/disabled, live level change.
- `tests/test_config_manager.cpp`: `logging` defaults, get/set, invalid-level normalization, persistence.
- `tests/test_settings_dialog.cpp`: log controls load, checkbox toggles level combo enablement, level change persists, Cancel reverts.

## Performance Note

At `VERB` the breathing animation logs every frame, which is I/O-heavy; enable it only temporarily for diagnosis. At the default `WARN`, the `shouldLog` macro short-circuit makes the high-frequency path **zero-overhead** (no message string built, no file write).
