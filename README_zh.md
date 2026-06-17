# trafficlight4ai

AI 编码工具的可视化红绿灯状态指示器（支持 Codex、Claude Code、Qoder CN、Copilot、Gemini 等）。

[English](README.md)

## 功能简介

当 AI 编码助手在终端中运行时，trafficlight4ai 让你一眼看到当前状态：

- **红灯闪烁** — AI 正在工作（工具调用、子代理、思考中）
- **黄灯闪烁** — AI 需要你确认（权限请求、通知）
- **绿灯常亮** — 完成，等待你的下一个指令

展现形式：桌面上的悬浮小窗口（始终置顶）+ 系统托盘图标。

## 工作原理

```
AI 工具 Hooks → tl4ai-ctl (CLI) → 本地 IPC socket → trafficlight4ai (GUI)
```

AI 工具的 hook 机制在合适的时机触发 `tl4ai-ctl red/yellow/green`，GUI 即时更新。

## 下载

预编译包可从 GitHub Releases 下载：

- [`trafficlight4ai-1.2.1-windows-amd64.zip`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-windows-amd64.zip) — Windows 构建，已通过 `windeployqt` 打包 Qt 运行库。
- [`trafficlight4ai-1.2.1-linux-amd64.deb`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-linux-amd64.deb) — Ubuntu/Debian 安装包。
- [`trafficlight4ai-1.2.1-fedora-amd64.rpm`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-fedora-amd64.rpm) — Fedora 安装包。
- [`trafficlight4ai-1.2.1-opensuse-amd64.rpm`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-opensuse-amd64.rpm) — openSUSE Leap 安装包。
- [`trafficlight4ai-1.2.1-arch-amd64.pkg.tar.zst`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-arch-amd64.pkg.tar.zst) — Arch Linux 安装包。
- [`trafficlight4ai-1.2.1-linux-amd64.AppImage`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-linux-amd64.AppImage) — 通用 Linux AppImage。
- [`trafficlight4ai-1.2.1-macos-arm64.zip`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-macos-arm64.zip) — macOS arm64 应用包，含 Qt frameworks。
- [`SHA256SUMS.txt`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/SHA256SUMS.txt) — 发布包校验和。

Linux 发行版安装包是动态链接构建，主要面向对应发行版家族。其它发行版可优先使用 AppImage，或按 [docs/BUILD_zh.md](docs/BUILD_zh.md) 从源码编译。macOS（最低 macOS 12）发布包已在 Release workflow 中支持，会从 macOS 支持合入后的新版本开始生成。

Windows（最低 Windows 10）上运行 `bin/trafficlight4ai.exe`。它会作为 GUI 程序启动，不应弹出命令行窗口。可在 PowerShell、cmd 或 AI 工具 hooks 中调用 `bin/tl4ai-ctl.exe red/yellow/green` 来切换灯状态。如果 Windows 提示缺少 MSVC 运行时 DLL，请安装 Microsoft Visual C++ Redistributable 2022 x64。

## 从源码编译

Linux、macOS 和 Windows 的编译前提、编译命令、打包注意事项和验证方式见 [docs/BUILD_zh.md](docs/BUILD_zh.md)。

源码构建已在 GitHub Actions 中验证 Ubuntu 24.04、Fedora 41、Arch Linux latest、openSUSE Leap 15.6、AppImage（Ubuntu 22.04）、macOS arm64 和 Windows。构建文档中列出了各发行版依赖包名、macOS 打包注意事项和 openSUSE 的 GCC 13 要求。

### CI/CD 工作流

| 工作流 | 用途 | 触发方式 |
|--------|------|---------|
| `build.yml` | 编译验证 + 测试，可选平台，不生成安装包 | PR 到 main 自动触发 / 手动选平台 |
| `package-jobs.yml` | 可复用工作流，包含 7 个平台的打包 job | 被其他工作流调用，不直接触发 |
| `package-test.yml` | 打包验证，可选平台，生成可下载的 artifact，不创建 Release | 手动触发（选平台 + 填版本号） |
| `release-packages.yml` | 正式发布：全平台打包 + 创建 GitHub Release + 上传资产 | 手动触发（版本号 + notes） |

`package-test.yml` 和 `release-packages.yml` 都调用 `package-jobs.yml`，后者额外包含 Release 创建步骤。

编译后生成两个可执行文件：

| 可执行文件 | 路径 | 说明 |
|---|---|---|
| `trafficlight4ai` | `build/src/trafficlight4ai` | Qt GUI 主程序 — 悬浮窗口 + 系统托盘图标，内嵌 IPC 服务端接收状态指令 |
| `tl4ai-ctl` | `build/tools/tl4ai-ctl` | 轻量 CLI — 通过本地 IPC socket 向 GUI 发送 `RED`/`YELLOW`/`GREEN` 指令 |

## 快速开始

### 1. 启动 GUI

```bash
./build/src/trafficlight4ai &
```

### 2. 让 hooks 能找到 tl4ai-ctl

Hooks 需要能找到 `tl4ai-ctl`，以下两种方式任选其一：

**方式 A** — 在 hook 命令中直接使用绝对路径（无需安装）：

在第 3 步的配置中，将 `tl4ai-ctl` 替换为完整路径，例如：

```
/home/you/trafficlight4ai/build/tools/tl4ai-ctl red
```

**方式 B** — 加入 PATH（这样 hooks 可以直接用短名 `tl4ai-ctl`）：

```bash
# 创建符号链接到 PATH 目录
sudo ln -s "$(pwd)/build/tools/tl4ai-ctl" /usr/local/bin/tl4ai-ctl

# 或复制到 ~/.local/bin（无需 sudo，确保 ~/.local/bin 已在 PATH 中）
cp build/tools/tl4ai-ctl ~/.local/bin/
```

**方式 C** — AppImage：直接用真实 `.AppImage` 路径加状态词。包内的 `tl4ai-ctl` 位于临时挂载点（`/tmp/.mount_*`），每次启动都会变化，切勿在保存的 hook 中引用它。把第 3 步配置中的 `tl4ai-ctl` 替换为设置对话框显示的 AppImage 路径，例如：

```
/home/you/Downloads/trafficlight4ai-1.2.1-linux-amd64.AppImage red
```

### 3. 配置 AI 工具的 hooks

各 AI 工具（Codex、Claude Code、Qoder CN、Copilot、Gemini）的 hooks 配置示例请参阅 [docs/HOOKS_zh.md](docs/HOOKS_zh.md)。也可以在设置对话框中直接查看和编辑模板。

### 4. 手动测试

```bash
tl4ai-ctl red      # 红灯闪烁
tl4ai-ctl yellow   # 黄灯闪烁
tl4ai-ctl green    # 绿灯常亮
```

## 功能特性

- **悬浮窗口** — 可拖动、始终置顶、记忆位置
- **系统托盘图标** — 颜色随状态变化，右键菜单
- **设置对话框** — 实时预览，取消可撤销
  - AI 工具选择（Codex / Claude Code / Qoder CN / Copilot / Gemini）
  - 超时自动回绿灯（默认 5 分钟，0 禁用）
  - 窗口大小（超小 / 小 / 中 / 大 / 超大）
  - 动画模式（呼吸灯 / 经典闪烁）
  - 动画周期（200~5000ms）
  - Socket 路径
- **策略模式** — 轻松添加新 AI 工具（实现 `AiToolStrategy` 接口，注册到 `AiToolRegistry`）
- **提示音** — 黄灯（需确认）和绿灯（完成）时播放提示音，支持 WAV/MP3/OGG，fallback 系统 beep
- **国际化** — 英语（默认）、中文、日语，运行时切换
- **轻量 CLI** — `tl4ai-ctl` 使用 Qt Core/Network 和 `QLocalSocket` 实现跨平台本地 IPC

## 配置文件

路径：`~/.config/trafficlight4ai/config.json`

```json
{
  "language": "en",
  "aiTool": "codex",
  "timeoutSec": 300,
  "window": {
    "size": "medium",
    "posX": 20,
    "posY": 20
  },
  "animation": {
    "mode": "breathing",
    "periodMs": 1000
  },
  "socket": {
    "path": "$XDG_RUNTIME_DIR/trafficlight4ai.sock"
  },
  "sound": {
    "yellowEnabled": true,
    "greenEnabled": true,
    "yellowFile": "",
    "greenFile": ""
  }
}
```

### 字段说明

| 字段 | 默认值 | 说明 |
|------|--------|------|
| `language` | `"en"` | 界面语言（`en` / `zh` / `ja`） |
| `aiTool` | `"codex"` | AI 工具（`codex` / `claude-code` / `qoder-cn` / `copilot` / `gemini`） |
| `timeoutSec` | `300` | 超时回绿灯秒数，0 禁用 |
| `window.size` | `"medium"` | 窗口大小（`xsmall` / `small` / `medium` / `large` / `xlarge`） |
| `window.posX` / `posY` | `20` | 窗口位置 |
| `animation.mode` | `"breathing"` | 动画模式（`breathing` / `classic`） |
| `animation.periodMs` | `1000` | 动画周期（200~5000 ms） |
| `socket.path` | 平台相关 | 见下方说明 |
| `sound.yellowEnabled` / `greenEnabled` | `true` | 提示音开关 |
| `sound.yellowFile` / `greenFile` | `""` | 自定义音效路径（WAV/MP3/OGG），空用系统 beep |

默认 socket 路径因平台而异：Linux 使用 `$XDG_RUNTIME_DIR/trafficlight4ai.sock`（fallback `/tmp/trafficlight4ai-$UID.sock`），macOS 使用 `$TMPDIR/trafficlight4ai.sock`，Windows 使用命名管道 `trafficlight4ai`。也可通过 `TL4AI_SOCKET` 环境变量覆盖。

## 测试

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
or
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

## 添加新 AI 工具

trafficlight4ai 使用策略模式支持多种 AI 工具。添加新工具只需：

1. **创建策略类** — 在 `src/AiToolStrategy.h` 中继承 `AiToolStrategy` 并实现：
   - `id()` — 唯一标识符（如 `"my-tool"`）
   - `displayName()` — 设置对话框中显示的名称（如 `"My Tool"`）
   - `defaultTimeoutSec()` — 自动回绿灯的超时秒数
   - `hooksTemplate()` — 该工具推荐的 hooks 配置 JSON

2. **注册** — 在同文件的 `AiToolRegistry::strategies()` 中添加静态实例并追加到列表。

3. **重新编译** — 新工具会自动出现在设置对话框的 AI 工具下拉列表中，其 hooks 模板可通过"查看推荐 Hooks 配置"按钮查看。

无需修改 IPC、状态机或 GUI 代码，策略模式会处理一切。

## GitHub 个人访问令牌（PAT）权限

如果使用 `GH_TOKEN` 推送代码或触发发布 workflow，PAT 需要以下权限：

### Fine-grained tokens（推荐）

| 权限 | 访问级别 | 用途 |
|------|---------|------|
| Contents | Read and write | 推送代码、更新 Release 说明 |
| Actions | Read and write | 触发发布 workflow |
| Metadata | Read-only | 默认必需 |

**Repository access** 设为 "Only select repositories" 并选择本仓库。

### Tokens (classic)

| Scope | 用途 |
|-------|------|
| `repo` | 推送代码、更新 Release |
| `workflow` | 触发 workflow dispatch（推送 `.github/workflows/` 变更时也需要） |

## 许可证

MIT
