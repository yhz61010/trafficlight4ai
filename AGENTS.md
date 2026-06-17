# Repository Guidelines

## 项目结构与模块组织

本项目是 Qt 6 桌面工具，使用 CMake 和 C++17 构建，目标平台为 Linux、macOS 12+ 和 Windows 10+。`src/` 包含 GUI 主程序与核心逻辑：`tl4ai_core` 覆盖 `StateManager`、`ConfigManager`、`IpcServer`、`Tl4aiClient`，`AiToolStrategy` 封装 Codex、Claude Code、Qoder CN、Copilot、Gemini 的 hooks 差异，`TrafficLightWidget`、`FloatingWindow`、`TrayIcon`、`SettingsDialog`、`SoundUtils` 负责界面、托盘、设置和提示音。`tools/` 提供 CLI `tl4ai-ctl`，通过共享的 `Tl4aiClient` 和 Qt `QLocalSocket` 向 GUI 发送状态；GUI 主程序也支持 `red`/`yellow`/`green` CLI 转发模式，供 AppImage hooks 直接调用稳定的 `.AppImage` 路径。`tests/` 是 Qt Test/CTest 测试。`resources/images/` 存放 PNG 图标，`resources/effects/` 存放内置 OGG 提示音，`translations/` 存放中文和日语 `.ts` 翻译，`packaging/linux/` 存放 deb/rpm/AppImage/Arch 打包脚本，`packaging/macos/` 存放 macOS zip 打包脚本，`docs/` 存放构建、hooks、问题记录和设计说明，`.claude/rules/` 与 `.claude/memory/` 存放 Claude 专用协作规则和记忆。

## 构建与验证文档

Linux、macOS 和 Windows 的编译前提、编译命令、注意事项与验证方式统一维护在 [docs/BUILD_zh.md](docs/BUILD_zh.md)，英文版为 [docs/BUILD.md](docs/BUILD.md)。不要在 README、AGENTS 或 CLAUDE 中重复粘贴完整编译步骤；编译流程变化时优先更新这两份构建文档。`Build` workflow 验证 Ubuntu 24.04、Fedora 41、Arch Linux latest、openSUSE Leap 15.6、AppImage、macOS arm64 和 Windows；`Package Test` workflow 通过复用 `Package Jobs` 生成可下载 artifact；`Release Packages` workflow 生成 Release 资产并创建 GitHub Release。openSUSE Leap 15.6 必须使用 `gcc13/gcc13-c++` 并以 `CC=gcc-13 CXX=g++-13` 配置 CMake。AppImage 打包在 Ubuntu 22.04 上运行，并在最终提取出的 AppImage 中校验 bundled GStreamer 插件、`gst-plugin-scanner` 和缺失依赖。GitHub Actions 中优先使用 Node 24 兼容的官方 actions 版本，例如 `actions/checkout@v6`、`actions/upload-artifact@v6` 和 `actions/download-artifact@v7`。

## 代码风格与命名约定

使用四空格缩进、C++17 和 Qt 惯用写法。类名用 `PascalCase`，方法、变量和测试槽函数用 `camelCase`，私有成员使用 `m_` 前缀，头文件使用 `#pragma once`。头文件中用到的 Qt 类型必须显式 `#include`。Qt signal/slot 使用新式连接语法。优先使用 RAII 和 Qt 父子对象生命周期管理，避免裸 `new/delete`、悬空指针和不必要的共享所有权。所有可见 UI 文本必须用 `tr()` 包裹，并同步更新 `translations/trafficlight4ai_zh.ts` 和 `translations/trafficlight4ai_ja.ts`。新增 GUI 设置应保持实时预览、取消可回滚的现有行为。

## 测试指南

测试基于 Qt Test，并通过 `tests/CMakeLists.txt` 注册。新增纯核心测试优先使用 `add_tl4ai_test(test_<component>)`；GUI 测试应链接 `tl4ai_gui` 并设置 `QT_QPA_PLATFORM=offscreen`。CLI 集成测试需要编译后的 `tl4ai-ctl`，AppImage CLI 转发行为覆盖在 `test_tl4ai_ctl` 中。测试文件命名为 `test_<component>.cpp`，测试槽函数按行为命名，例如 `duplicateCommandRefreshesTimeout`。修改状态流转、配置读写、IPC、CLI、AI 工具策略、窗口尺寸、动画、国际化、提示音或 AppImage hooks 路径解析时，应补充聚焦测试。IPC 客户端读取必须聚合到换行符或断开后再处理，避免分片写入丢命令。

## 文档同步

`README.md` 是英文文档，`README_zh.md` 是对应的中文文档。更新构建方式、功能说明、配置字段、运行行为或用户可见命令时，必须同时更新两份 README，保持内容一致。Hooks 配置示例维护在 [docs/HOOKS.md](docs/HOOKS.md) 和 [docs/HOOKS_zh.md](docs/HOOKS_zh.md)；新增或修改 AI 工具 hooks 时，同步更新这两份文档和 `AiToolStrategy` 模板，设置对话框会直接展示这些策略模板。AppImage hooks 必须使用真实 `.AppImage` 文件路径加状态词，不要把 `/tmp/.mount_*` 下的包内 `tl4ai-ctl` 写入持久配置。`CLAUDE.md` 和 `.claude/**` 是 Claude 专用指南与记忆，默认只作参考；其中影响本仓库的项目事实变化时，应同步反映到本文件。仅修复某一语言的表达问题时，可只改对应文件。

## 配置与资源注意事项

运行配置位于 `~/.config/trafficlight4ai/config.json`，socket 路径或名称可用 `TL4AI_SOCKET` 覆盖。当前配置包含 `language`（`en`/`zh`/`ja`）、`aiTool`（`codex`/`claude-code`/`qoder-cn`/`copilot`/`gemini`）、`timeoutSec`、`window.size`（`xsmall`/`small`/`medium`/`large`/`xlarge`）、`window.posX`/`window.posY`、`animation.mode`（`breathing`/`classic`）、`animation.periodMs`、`socket.path` 和 `sound.*`。默认 socket 路径是 Linux `$XDG_RUNTIME_DIR/trafficlight4ai.sock`（fallback `/tmp/trafficlight4ai-$UID.sock`）、macOS `$TMPDIR/trafficlight4ai.sock`、Windows 命名管道 `trafficlight4ai`；服务端 `ConfigManager` 与客户端 `Tl4aiClient` 的默认解析必须保持一致。不要提交个人配置、机器相关 socket 路径或本机音频路径。新增图片请放入 `resources/images/` 并在 Qt 资源配置中注册；新增内置提示音请放入 `resources/effects/` 并在 Qt 资源配置中注册。

## 实现注意事项

Debug 构建保留 `qDebug()` 输出；Release 构建通过 `QT_NO_DEBUG_OUTPUT` 移除调试日志，但 warning 和 critical 日志仍保留。Windows GUI 主程序必须保持 `WIN32_EXECUTABLE TRUE`，避免运行时弹出命令行窗口；macOS GUI 主程序必须保持 `MACOSX_BUNDLE TRUE`，并通过 `macdeployqt` 部署 Qt frameworks。GUI 主程序的 CLI 转发分支必须在构造 `QApplication` 前处理，并复用 `Tl4aiClient`，避免复制 socket/IPC 逻辑。AppImage 的 `initBundledGStreamerPlugins()` 应在 `QApplication` 构造后、任何 `QMediaPlayer` 使用前执行；它需要隔离自带 GStreamer 插件路径、scanner 和 registry，避免扫描宿主插件。`QLocalServer::listen()` 前需拒绝被普通文件占用的 socket 路径。悬浮窗口尺寸切换后需要在布局重算后恢复位置；托盘图标在部分桌面环境中需要延迟重设才能可靠刷新。处理外部输入、配置文件、socket 路径、自定义音频路径和 hook 命令路径时要显式校验并避免泄露本机敏感路径。

## 发布注意事项

GitHub Release 由 `Release Packages` workflow 生成并发布，资产应包含 Windows zip、macOS arm64 zip、Ubuntu/Debian `.deb`、Fedora `.rpm`、openSUSE `.rpm`、Arch `.pkg.tar.zst`、通用 Linux `.AppImage` 和 `SHA256SUMS.txt`。发布或更新 Release 资产后，必须同步 `README.md` 和 `README_zh.md` 中的具体版本号和下载链接；`docs/BUILD.md` 与 `docs/BUILD_zh.md` 当前使用 `<version>` 占位符和 GitHub Releases 页面链接，只有资产命名、平台矩阵、打包方式或验证方式变化时才需要同步。Linux 打包脚本位于 `packaging/linux/`，依赖 CMake install 规则暂存二进制、desktop 文件、图标、README 和许可证。macOS 打包脚本位于 `packaging/macos/`，输出 `.app` bundle、`docs/` 和 `bin/tl4ai-ctl` 包装脚本。AppImage 脚本固定 linuxdeploy/type2-runtime 版本并做 SHA256 校验，还会捆绑 Qt Multimedia/GStreamer 运行时组件并在打包后验证必需插件、scanner 与依赖完整性；排查 AppImage 音频或 hooks 问题时以最终 `squashfs-root` 为准，不只看中间 `AppDir`。`dist/` 是本地发布产物目录，已被 `.gitignore` 忽略，不要提交。

## 提交与 Pull Request 规范

Git 历史使用简洁的 conventional-style 前缀，如 `feat:`、`fix:`、`refactor:`、`docs:`、`test:`、`chore:`、`perf:`、`ci:`。标题使用祈使语气并说明具体改动，例如 `fix: resize floating window geometry after size changes`。Pull Request 应基于完整分支 diff 编写，包含问题说明、改动摘要和测试结果；涉及可见 UI、图片、窗口行为或动画变化时附截图或录屏，并链接相关 issue 或设计文档。
