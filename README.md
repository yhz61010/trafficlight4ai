# trafficlight4ai

A visual traffic light status indicator for AI coding tools (Codex, Claude Code, Qoder CN, Copilot, Gemini, and more).

[中文文档](README_zh.md)

## What It Does

When your AI coding assistant is running in the terminal, trafficlight4ai shows you what's happening at a glance:

- **Red (blinking)** — AI is working (tool calls, subagents, thinking)
- **Yellow (blinking)** — AI needs your confirmation (permission requests, notifications)
- **Green (steady)** — Done, waiting for your next prompt

It displays as a small floating always-on-top window + system tray icon on your desktop.

## How It Works

```
AI Tool Hooks → tl4ai-ctl (CLI) → local IPC socket → trafficlight4ai (GUI)
```

Your AI tool's hook system triggers `tl4ai-ctl red/yellow/green` at the right moments. The GUI updates instantly.

## Download

Prebuilt archives are available from GitHub Releases:

- [`trafficlight4ai-1.2.1-windows-amd64.zip`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-windows-amd64.zip) — Windows build with Qt runtime files bundled by `windeployqt`.
- [`trafficlight4ai-1.2.1-linux-amd64.deb`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-linux-amd64.deb) — Ubuntu/Debian package.
- [`trafficlight4ai-1.2.1-fedora-amd64.rpm`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-fedora-amd64.rpm) — Fedora package.
- [`trafficlight4ai-1.2.1-opensuse-amd64.rpm`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-opensuse-amd64.rpm) — openSUSE Leap package.
- [`trafficlight4ai-1.2.1-arch-amd64.pkg.tar.zst`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-arch-amd64.pkg.tar.zst) — Arch Linux package.
- [`trafficlight4ai-1.2.1-linux-amd64.AppImage`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-linux-amd64.AppImage) — generic Linux AppImage.
- [`trafficlight4ai-1.2.1-macos-arm64.zip`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/trafficlight4ai-1.2.1-macos-arm64.zip) — macOS arm64 app bundle with Qt frameworks.
- [`SHA256SUMS.txt`](https://github.com/yhz61010/trafficlight4ai/releases/download/v1.2.1/SHA256SUMS.txt) — checksums for release archives.

Linux distro packages are dynamically linked and target the named distribution family. For other distributions, use the AppImage or build from source using [docs/BUILD.md](docs/BUILD.md). macOS (12 or later) release packages are supported by the release workflow for new releases after macOS support lands.

On Windows (10 or later), run `bin/trafficlight4ai.exe`. It is built as a GUI application and should not open a console window. Use `bin/tl4ai-ctl.exe red/yellow/green` from PowerShell, cmd, or AI tool hooks to change the light state. If Windows reports missing MSVC runtime DLLs, install the Microsoft Visual C++ Redistributable 2022 x64.

## Build From Source

See [docs/BUILD.md](docs/BUILD.md) for Linux, macOS, and Windows prerequisites, build commands, packaging notes, and verification steps.

Source builds are verified in GitHub Actions on Ubuntu 24.04, Fedora 41, Arch Linux latest, openSUSE Leap 15.6, AppImage (Ubuntu 22.04), macOS arm64, and Windows. The build guide lists distribution-specific packages, macOS packaging notes, and the openSUSE GCC 13 requirement.

### CI/CD Workflows

| Workflow | Purpose | Trigger |
|----------|---------|---------|
| `build.yml` | Compile verification + tests, selectable platform, no packages | PR to main (auto) / manual |
| `package-jobs.yml` | Reusable workflow with 7 platform packaging jobs | Called by other workflows |
| `package-test.yml` | Package verification, selectable platform, generates downloadable artifacts without creating a Release | Manual (select platform + version) |
| `release-packages.yml` | Full release: all-platform packaging + GitHub Release creation + asset upload | Manual (version + notes) |

`package-test.yml` and `release-packages.yml` both call `package-jobs.yml`; the latter adds a Release creation step.

The build produces two executables:

| Executable | Path | Description |
|---|---|---|
| `trafficlight4ai` | `build/src/trafficlight4ai` | Qt GUI main program — floating window + system tray icon, embeds IPC server to receive state commands |
| `tl4ai-ctl` | `build/tools/tl4ai-ctl` | Lightweight CLI — sends `RED`/`YELLOW`/`GREEN` commands to the GUI via local IPC socket |

## Quick Start

### 1. Run the GUI

```bash
./build/src/trafficlight4ai &
```

### 2. Make tl4ai-ctl available to hooks

Hooks need to find `tl4ai-ctl`. Choose one of the following:

**Option A** — Use absolute path directly in hook commands (no installation needed):

Replace `tl4ai-ctl` with the full path in step 3, e.g.:

```
/home/you/trafficlight4ai/build/tools/tl4ai-ctl red
```

**Option B** — Add to PATH (so hooks can use the short name `tl4ai-ctl`):

```bash
# Symlink to a directory already in PATH
sudo ln -s "$(pwd)/build/tools/tl4ai-ctl" /usr/local/bin/tl4ai-ctl

# Or copy to ~/.local/bin (no sudo needed, make sure ~/.local/bin is in PATH)
cp build/tools/tl4ai-ctl ~/.local/bin/
```

**Option C** — AppImage: use the real `.AppImage` path with the state word directly. The bundled `tl4ai-ctl` lives in a temporary mount (`/tmp/.mount_*`) that changes on every launch, so never reference it in a saved hook. Replace `tl4ai-ctl` with the AppImage path shown by the Settings dialog, e.g.:

```
/home/you/Downloads/trafficlight4ai-1.2.1-linux-amd64.AppImage red
```

### 3. Configure hooks for your AI tool

See [docs/HOOKS.md](docs/HOOKS.md) for hooks configuration examples for each supported AI tool (Codex, Claude Code, Qoder CN, Copilot, Gemini). You can also view and edit these templates directly in the Settings dialog.

### 4. Test manually

```bash
tl4ai-ctl red      # Red light blinks
tl4ai-ctl yellow   # Yellow light blinks
tl4ai-ctl green    # Green light steady
```

## Features

- **Floating window** — Draggable, always-on-top, remembers position
- **System tray icon** — Color changes with state, right-click menu
- **Settings dialog** — Live preview, cancel to undo
  - AI tool selection (Codex / Claude Code / Qoder CN / Copilot / Gemini)
  - Timeout auto-idle (default 5 min, 0 to disable)
  - Window size (extra small / small / medium / large / extra large)
  - Animation mode (breathing / classic blink)
  - Animation period (200–5000ms)
  - Socket path
- **Strategy pattern** — Easy to add new AI tools (implement `AiToolStrategy`, register in `AiToolRegistry`)
- **Sound notifications** — Plays sound on yellow (confirmation needed) and green (done), supports WAV/MP3/OGG with system beep fallback
- **i18n** — English (default), Chinese, Japanese, switchable at runtime
- **Lightweight CLI** — `tl4ai-ctl` uses Qt Core/Network and `QLocalSocket` for cross-platform local IPC

## Configuration

Config file: `~/.config/trafficlight4ai/config.json`

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

### Fields

| Field | Default | Description |
|-------|---------|-------------|
| `language` | `"en"` | UI language (`en` / `zh` / `ja`) |
| `aiTool` | `"codex"` | AI tool (`codex` / `claude-code` / `qoder-cn` / `copilot` / `gemini`) |
| `timeoutSec` | `300` | Seconds before auto-idle (0 to disable) |
| `window.size` | `"medium"` | Window size (`xsmall` / `small` / `medium` / `large` / `xlarge`) |
| `window.posX` / `posY` | `20` | Window position |
| `animation.mode` | `"breathing"` | Animation mode (`breathing` / `classic`) |
| `animation.periodMs` | `1000` | Animation period (200–5000 ms) |
| `socket.path` | platform-specific | See below |
| `sound.yellowEnabled` / `greenEnabled` | `true` | Sound notification toggle |
| `sound.yellowFile` / `greenFile` | `""` | Custom sound file path (WAV/MP3/OGG); empty = system beep |

The default socket path is platform-specific: Linux uses `$XDG_RUNTIME_DIR/trafficlight4ai.sock` (fallback `/tmp/trafficlight4ai-$UID.sock`), macOS uses `$TMPDIR/trafficlight4ai.sock`, and Windows uses the named pipe `trafficlight4ai`. It can also be overridden via `TL4AI_SOCKET` environment variable.

## Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && cd build && ctest --output-on-failure
or
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
cd build && ctest --output-on-failure
```

## Adding a New AI Tool

trafficlight4ai uses a strategy pattern to support multiple AI tools. To add a new one:

1. **Create a strategy class** in `src/AiToolStrategy.h` — inherit from `AiToolStrategy` and implement:
   - `id()` — unique identifier (e.g. `"my-tool"`)
   - `displayName()` — name shown in the Settings dialog (e.g. `"My Tool"`)
   - `defaultTimeoutSec()` — auto-idle timeout in seconds
   - `hooksTemplate()` — recommended hooks config JSON for this tool

2. **Register it** in `AiToolRegistry::strategies()` (same file) — add a static instance and append its pointer to the list.

3. **Rebuild** — the new tool will appear in the Settings dialog's AI tool dropdown and its hooks template will be available via "View Recommended Hooks Config".

No changes are needed to IPC, state machine, or GUI code — the strategy pattern handles everything.

## GitHub Personal Access Token (PAT) Permissions

If you use `GH_TOKEN` to push code or trigger release workflows, your PAT needs the following permissions:

### Fine-grained tokens (recommended)

| Permission | Access | Purpose |
|------------|--------|---------|
| Contents | Read and write | Push commits, update release notes |
| Actions | Read and write | Trigger release workflow |
| Metadata | Read-only | Required by default |

Set **Repository access** to "Only select repositories" and choose this repo.

### Tokens (classic)

| Scope | Purpose |
|-------|---------|
| `repo` | Push code and update releases |
| `workflow` | Trigger workflow dispatch (also needed when pushing changes to `.github/workflows/`) |

## License

MIT
