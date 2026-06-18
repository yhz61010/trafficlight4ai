# 项目目录结构

```
trafficlight4ai/
├── CMakeLists.txt                 # 根构建文件
├── CLAUDE.md                      # 项目指南
├── .github/workflows/             # CI（build.yml 编译验证 + release-packages.yml 发布打包）
├── docs/
│   ├── BUILD.md / BUILD_zh.md     # 跨平台构建指南（双语）
│   ├── HOOKS.md / HOOKS_zh.md     # AI 工具 Hooks 配置示例（双语）
│   ├── KNOWN_ISSUES.md            # 已知平台问题
│   └── PROJECT_STRUCTURE.md       # 本文件
├── src/
│   ├── CMakeLists.txt             # tl4ai_core 静态库 + trafficlight4ai 可执行文件
│   ├── StateManager.h/cpp         # 状态机（纯逻辑，含超时机制）
│   ├── ConfigManager.h/cpp        # JSON 配置管理
│   ├── IpcServer.h/cpp            # QLocalServer 本地 IPC 服务端
│   ├── Tl4aiClient.h/cpp          # 共享 IPC 客户端（socket 路径解析 + 发送，供 tl4ai-ctl 与 GUI CLI 复用）
│   ├── Logger.h/cpp              # 集中式分级日志器（文件 + 控制台，大小滚动）
│   ├── AiToolStrategy.h           # AI 工具策略接口 + Registry
│   ├── TrafficLightWidget.h/cpp   # 红绿灯绘制控件
│   ├── FloatingWindow.h/cpp       # 可拖动悬浮窗口
│   ├── TrayIcon.h/cpp             # 系统托盘图标
│   ├── SettingsDialog.h/cpp       # 设置对话框（实时预览+取消撤销）
│   ├── SoundUtils.h/cpp           # 音效播放工具（QMediaPlayer + beep fallback）
│   └── main.cpp                   # 入口
├── tests/
│   ├── CMakeLists.txt
│   ├── test_state_manager.cpp     # StateManager 单元测试
│   ├── test_config_manager.cpp    # ConfigManager 单元测试
│   ├── test_ipc_server.cpp        # IPC 协议集成测试
│   ├── test_ai_tool_strategy.cpp  # AiToolStrategy 单元测试
│   ├── test_traffic_light_widget.cpp  # TrafficLightWidget 单元测试（offscreen）
│   └── test_tl4ai_ctl.cpp         # CLI 集成测试
├── tools/
│   ├── CMakeLists.txt
│   └── tl4ai_ctl.cpp              # Qt QLocalSocket CLI
├── translations/
│   ├── trafficlight4ai_zh.ts      # 中文翻译
│   └── trafficlight4ai_ja.ts      # 日语翻译
├── packaging/
│   ├── linux/                     # deb/rpm/AppImage/Arch 打包脚本 + .desktop
│   └── macos/                     # macOS zip 打包脚本
└── resources/
    ├── resources.qrc
    └── images/                    # 红绿灯 PNG + 应用图标
```

## 核心类

| 类 | 职责 | 依赖 |
|---|------|------|
| `StateManager` | 状态机（Working/WaitingConfirm/Idle），超时自动回 Idle | Qt Core |
| `ConfigManager` | JSON 配置读写（`~/.config/trafficlight4ai/config.json`） | Qt Core |
| `IpcServer` | QLocalServer 监听本地 IPC socket，解析指令转发给 StateManager | Qt Network |
| `Tl4aiClient` | 共享 IPC 客户端：默认 socket 路径解析、stdin drain、发送状态指令，供 tl4ai-ctl CLI 与 GUI 的 AppImage CLI 转发模式复用 | Qt Network |
| `Logger` | 集中式分级日志器（VERB/DEBUG/INFO/WARN/ERROR），单例，写文件 + stderr，大小上限滚动，级别由 config 控制 | Qt Core |
| `AiToolStrategy` | 策略接口，封装各 AI 工具的差异（hooks 模板、默认超时等） | 无 |
| `TrafficLightWidget` | 自定义 QWidget，绘制三灯 UI，支持呼吸灯/经典闪烁 | Qt Widgets |
| `FloatingWindow` | 无边框置顶窗口，可拖动，记忆位置 | Qt Widgets |
| `TrayIcon` | 系统托盘图标，颜色随状态变化，右键菜单 | Qt Widgets |
| `SettingsDialog` | 设置对话框，实时预览配置变更，取消可撤销，含查看/编辑 Hooks 配置 | Qt Widgets |
| `SoundUtils` | 音效播放（QMediaPlayer，支持 WAV/MP3/OGG，fallback 系统 beep） | Qt Multimedia |

## 测试

使用 Qt Test（QTest），每个测试为独立可执行文件：

| 测试 | 覆盖范围 |
|------|---------|
| `test_state_manager` | 状态切换、signal 发射、命令解析、超时机制 |
| `test_config_manager` | 默认值、读写持久化、容错回退、参数校验、aiTool/timeoutSec |
| `test_ipc_server` | socket 收发、无效指令、旧 socket 清理、restart |
| `test_ai_tool_strategy` | 事件名验证、hooks 模板内容、Registry 查找、hooksConfigPath/hooksIsEntireFile |
| `test_logger` | 级别字符串互转、级别过滤（shouldLog）、启用/禁用文件写入、运行时改级别 |
| `test_traffic_light_widget` | sizePresetFromString 字符串到枚举转换（需 offscreen 平台） |
| `test_tl4ai_ctl` | CLI 集成测试，需要编译后的 tl4ai-ctl 二进制 |
