# CLAUDE.md

本文件为 Claude Code (claude.ai/code) 提供项目上下文指南。

## 项目概述

trafficlight4ai 是一个 C++ Qt6 桌面应用，为 AI 编码工具（Codex、Claude Code、Qoder CN、Copilot、Gemini 等）提供可视化红绿灯状态指示器。

- **红灯闪烁**：AI 工具正在工作中
- **黄灯闪烁**：AI 工具等待用户确认
- **绿灯常亮**：工作完成

展现形式：系统托盘图标 + 可拖动悬浮小窗口（默认左上角）。

## 技术栈

- **语言**：C++17
- **GUI 框架**：Qt 6（Core, Widgets, Network, Multimedia, LinguistTools, Test）
- **构建系统**：CMake（最低 3.20）
- **目标平台**：Linux，Windows（最低 Windows 10），macOS（最低 macOS 12）
- **状态检测**：AI 工具 Hooks → tl4ai-ctl CLI → Qt local IPC socket
- **国际化**：Qt Linguist（QTranslator + .ts/.qm），支持英语/中文/日语

## 构建与验证

Linux 和 Windows 的编译前提、编译命令、注意事项与验证方式统一维护在 [docs/BUILD_zh.md](docs/BUILD_zh.md)，英文版为 [docs/BUILD.md](docs/BUILD.md)。不要在本文件中重复维护完整编译步骤。

常用快速命令（Linux）：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)  # 编译
cd build && ctest --output-on-failure                                       # 全部测试
cd build && ctest -R test_state_manager --output-on-failure                 # 单个测试
```

## 架构

### 两个可执行文件

- **trafficlight4ai**（`src/`）— Qt GUI 主进程，内嵌 IPC Server
- **tl4ai-ctl**（`tools/`）— Qt 轻量 CLI，通过 QLocalSocket 发送状态指令

目录结构、核心类及测试覆盖详见 [docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md)。

### 数据流

```
AI Tool Hooks → tl4ai-ctl (Qt CLI) → local IPC socket → IpcServer → StateManager → TrafficLightWidget + TrayIcon
```

### 库分层

`tl4ai_core`（静态库）包含 StateManager、ConfigManager、IpcServer，供测试和主程序共用。GUI 组件仅在主程序中链接。

### Socket 协议

Linux 默认路径：`$XDG_RUNTIME_DIR/trafficlight4ai.sock`（fallback `/tmp/trafficlight4ai-$UID.sock`）。macOS 默认路径：`$TMPDIR/trafficlight4ai.sock`（per-user 临时目录）。Windows 默认名称：`trafficlight4ai`。可通过 config.json 或 `TL4AI_SOCKET` 环境变量覆盖。

指令为单行纯文本：`RED\n` / `YELLOW\n` / `GREEN\n`，大小写不敏感，无法识别的指令静默忽略。

`tl4ai-ctl` 在 Linux 上启动时会非阻塞 drain stdin（AI 工具 hooks 会往 stdin 写 JSON 数据），Windows 上跳过。

AppImage：GUI 主程序 `trafficlight4ai` 也接受 `red`/`yellow`/`green` 参数（CLI 模式经 IPC 转发后退出、不启 GUI，因为 AppRun 执行的就是 GUI 二进制）。推荐 Hooks 用稳定的 `$APPIMAGE` 路径（由 `resolvedCtlPath()` 处理），勿用临时 `/tmp/.mount_*` 路径。客户端 socket/发送逻辑共享于 `Tl4aiClient`（tl4ai_core）。

配置文件路径：`~/.config/trafficlight4ai/config.json`，字段说明见 README.md 的 Configuration 章节。

### 日志系统

集中式分级日志器（`Logger`，`tl4ai_core`）：5 级 `VERB<DEBUG<INFO<WARN<ERROR`，默认 WARN，文件 + 控制台双写、大小滚动，级别由 config `logging` 字段控制，设置对话框可实时开关/选级别/打开日志目录。设计、配置与各模块埋点详见 [docs/LOGGING_zh.md](docs/LOGGING_zh.md)，英文版 [docs/LOGGING.md](docs/LOGGING.md)。

### AI 工具策略模式

`AiToolStrategy` 接口封装不同 AI 工具的差异（hooks 模板、配置路径、默认超时等），通过 `AiToolRegistry` 注册和查找。新增工具只需添加一个 Strategy 实现并注册到 Registry，详见 `src/AiToolStrategy.h`。

## 测试

使用 Qt Test（QTest），每个测试为独立可执行文件，覆盖范围详见 [docs/PROJECT_STRUCTURE.md](docs/PROJECT_STRUCTURE.md)。

### 注意事项

- 每个测试方法使用独立的临时文件路径（`init()` slot 生成唯一路径），避免测试间污染
- 构造 stale Unix socket 需用 POSIX `socket()/bind()/close()`，`QLocalServer::close()` 会自动 unlink
- IpcServer 测试中 `sendCommand` 后需等待事件循环处理，优先使用 `QTRY_COMPARE_WITH_TIMEOUT` 而非固定 `qWait`
- Qt Widgets 相关测试需在 CMake 中设置 `QT_QPA_PLATFORM=offscreen`，否则在 CI 无显示器环境下会 abort

## 已知平台问题

详见 [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md)。

## 代码约定

- Qt 信号槽使用新式语法（`&Class::signal`）
- 头文件使用 `#pragma once`
- 头文件中使用的 Qt 类型必须显式 `#include`，不要依赖间接包含（CI 容器环境下隐式包含可能不存在）
- 所有 UI 文本使用 `tr()` 包裹（国际化），同步更新 `translations/` 下的 `.ts` 文件
- 新增测试使用 `add_tl4ai_test(test_<component>)` 宏注册，测试槽函数按行为命名（如 `duplicateCommandRefreshesTimeout`）
- Git commit message 使用英文编写，conventional-style 前缀：`feat:` / `fix:` / `refactor:` / `docs:` / `test:` / `chore:` / `perf:` / `ci:` / `debug:`，标题用祈使语气
- 代码注释使用英文编写
- IpcServer 客户端读取需聚合至换行符或断开，不可在首次 `readyRead` 后立即处理（防止分片写入丢命令）
- 发布新 Release 后，同步更新 README.md、README_zh.md 中的版本号和下载链接（BUILD 文档已用 `<version>` 占位符，无需更新）
- 更新 README.md 时必须同步更新 README_zh.md，保持双语内容一致
- 修改外迁文档（`docs/PROJECT_STRUCTURE.md`、`docs/KNOWN_ISSUES.md`）时需与代码实际状态保持一致
- `AiToolStrategy.h` 的 hooks 模板是 JSON：含空格的命令路径必须 JSON 转义为 `\"...\"`，不能用裸引号，否则生成非法 JSON
- 通过 workflow_dispatch 触发 `release-packages.yml` 时，`notes` 输入严禁含反引号（会被内联进 bash 当命令替换，导致构建失败）

## 外部文档路径

本项目的 AI 生成文档统一管理在项目 `docs/` 目录下：

| 路径 | 用途 |
|------|------|
| `docs/superpowers/` | superpowers 插件产物（specs/、plans/） |

> superpowers 生成的文档（specs/、plans/ 等）一律使用中文编写。

## AI 交互规则

- 若有不明白或不明确的地方，一定要先问我。不要自己幻想或无中生有。
- 用户偏好使用中文对话。

## 项目记忆

项目级记忆存储在 `.claude/memory/` 目录中，包含跨会话的协作约定和工作流偏好。

新会话开始时，请先读取 `.claude/memory/MEMORY.md` 了解已有的记忆内容。
