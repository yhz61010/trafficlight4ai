# 日志系统

本文说明 trafficlight4ai 的日志系统设计、配置方式与各模块埋点。英文版为 [LOGGING.md](LOGGING.md)。

## 概述

trafficlight4ai 提供一套集中式分级日志器，可在设置对话框中开关、选择级别并查看日志文件路径。日志同时写入**文件**与**控制台（stderr）**，并按大小自动滚动。日志器独立于 Qt 的 `qDebug`/`qWarning`（后者在 Release 构建被 `QT_NO_DEBUG_OUTPUT` 剥离），因此 **Release 构建也能正常写日志文件**。

## 日志级别

从最详细到最严重，共 5 级：

```
VERB < DEBUG < INFO < WARN < ERROR
```

- 设置某级别后，**仅记录该级别及更严重**的日志（例如默认 `WARN` 只记录 WARN/ERROR）。
- **默认级别：`WARN`**。
- `VERB` 为最详细级别，会记录每次呼吸灯透明度变化（高频）。

| 级别 | 用途 |
|------|------|
| `VERB` | 高频细节：呼吸灯透明度变化、音效播放结束清理 |
| `DEBUG` | 内部状态流转：控件/托盘状态切换、IPC 命令接收、动画启动 |
| `INFO` | 重要业务事件：状态变化、音效播放/设置变更、程序启动 |
| `WARN` | 可恢复异常：未知 IPC 命令、音效 URL 无效回退系统 beep |
| `ERROR` | 严重错误：IPC 监听失败、音效播放失败 |

## 日志文件

- **路径**（固定，不可在配置中自定义，仅在设置中展示）：
  `<应用数据目录>/logs/trafficlight4ai.log`
  - Linux：`~/.local/share/trafficlight4ai/logs/trafficlight4ai.log`
- **滚动**：单文件超过约 **5 MB** 时，重命名为 `trafficlight4ai.log.1`（覆盖旧备份），保留 **1 个**备份。
- **每行格式**：

  ```
  2026-06-18 12:34:56.789 [WARN] [Sound] Invalid sound url for 'x.ogg', falling back to system beep
  ```

  即 `时间戳(毫秒) [级别] [分类] 消息`。

## 配置

配置文件 `~/.config/trafficlight4ai/config.json` 中新增 `logging` 对象：

```json
"logging": {
  "enabled": true,
  "level": "warn"
}
```

| 字段 | 默认 | 说明 |
|------|------|------|
| `logging.enabled` | `true` | 是否写入日志（文件 + 控制台） |
| `logging.level` | `"warn"` | 最低级别：`verb` / `debug` / `info` / `warn` / `error`，非法值回退 `warn` |

### 设置对话框

设置对话框新增三行：

- **Logging（日志）**：开关复选框；关闭时联动禁用级别下拉。
- **Log Level（日志级别）**：`VERB` / `DEBUG` / `INFO` / `WARN` / `ERROR` 下拉。
- **Log File（日志文件）**：只读路径框 + 「Open Folder（打开目录）」按钮。

变更**实时生效**（同时写入 config 并应用到运行中的日志器），点击取消会经快照机制撤销。

## 架构

- **`Logger`（`src/Logger.h` / `Logger.cpp`，位于 `tl4ai_core`）**：单例。
  - `configure(enabled, level, filePath)` 在启动时一次性应用开关/级别/路径。
  - `setEnabled()` / `setLevel()` 供设置对话框实时调整。
  - `shouldLog(level)` 为**无锁快路径**（`m_enabled`、`m_level` 用 `std::atomic`），让高频调用方（呼吸灯）在级别被过滤时跳过消息字符串的构造。
  - 文件写入用 `QMutex` 保护；保持 `QFile` 常开、每行 `flush`（防崩溃丢日志），不做 `fsync`。
- **日志宏**：`TL_LOGV` / `TL_LOGD` / `TL_LOGI` / `TL_LOGW` / `TL_LOGE`，在构造消息串前先 `shouldLog` 短路。
- **Qt 消息转发**：`main.cpp` 安装 `qInstallMessageHandler`，把框架自身的 `qDebug`/`qWarning` 等按级别（Debug/Info/Warn/Error）转发进 `Logger`，与项目日志共用同一文件/控制台与级别过滤；项目自身的 `TL_LOG*` 直接走 `Logger`，不经此处理器，因此**不会重复**。
- **Release 行为**：`Logger` 不使用 Qt 的 `qDebug` 系列，故不受 `QT_NO_DEBUG_OUTPUT` 影响，Release 仍写文件。

## 埋点一览

| 模块 | 位置 | 级别 |
|------|------|------|
| `main.cpp` | 程序启动（含日志路径/级别） | INFO |
| `main.cpp` | 状态触发音效（黄/绿） | INFO |
| `StateManager` | 状态切换 | INFO |
| `StateManager` | 未知 IPC 命令 | WARN |
| `IpcServer` | 开始监听 / restart 成功 | INFO |
| `IpcServer` | 监听失败 / 路径被非 socket 占用 | ERROR |
| `IpcServer` | 收到命令 | DEBUG |
| `SoundUtils` | 播放入口（路径 + URL） | INFO |
| `SoundUtils` | URL 无效，回退系统 beep | WARN |
| `SoundUtils` | QMediaPlayer 播放错误 | ERROR |
| `SoundUtils` | 播放结束、清理 player | VERB |
| `SettingsDialog` | 音效开关（启用/禁用） | INFO |
| `SettingsDialog` | 音效文件选定（浏览） | INFO |
| `SettingsDialog` | 音效试听 | INFO（经 `SoundUtils`） |
| `TrafficLightWidget` | **呼吸灯透明度变化（`setActiveAlpha`）** | **VERB** |
| `TrafficLightWidget` | 状态切换 / 动画启动 | DEBUG |
| `TrayIcon` | 状态切换 / 图标设置 | DEBUG |

## 测试

- `tests/test_logger.cpp`：级别字符串互转（非法回退 WARN）、`shouldLog` 级别过滤、启用/禁用时的文件写入、运行时改级别生效。
- `tests/test_config_manager.cpp`：`logging` 默认值、读写、非法级别校验、持久化。
- `tests/test_settings_dialog.cpp`：日志控件加载、勾选联动级别下拉、级别变更落配置、取消撤销。

## 性能注意

`VERB` 级别下呼吸灯会逐帧写日志，I/O 偏重，仅建议排障时临时开启。默认 `WARN` 时，`shouldLog` 宏短路使高频路径**零额外开销**（不构造消息字符串、不触发文件写入）。
