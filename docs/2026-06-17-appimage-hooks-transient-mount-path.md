# AppImage 推荐 Hooks 配置暴露临时挂载路径的修复

> 日期：2026-06-17

## 背景 / 问题现象

在本地运行 AppImage 后，设置里"查看推荐 Hooks 配置"显示的 `tl4ai-ctl` 命令路径用了 AppImage 的临时挂载点：

```json
{
  "hooks": {
    "UserPromptSubmit": [
      { "hooks": [{ "type": "command", "command": "/tmp/.mount_traffiGKBfdA/usr/bin/tl4ai-ctl red" }] }
    ],
    "PreToolUse": [
      { "hooks": [{ "type": "command", "command": "/tmp/.mount_traffiGKBfdA/usr/bin/tl4ai-ctl red" }] }
    ],
    "Stop": [
      { "hooks": [{ "type": "command", "command": "/tmp/.mount_traffiGKBfdA/usr/bin/tl4ai-ctl green" }] }
    ]
  }
}
```

问题：`/tmp/.mount_traffiXXXXXX/` 是 AppImage 运行时的 FUSE 挂载点，`.mount_traffi` 后缀**每次启动都会随机变化**。用户若把这段配置复制进 Claude/Codex 等工具的 hooks 设置，下次重启 AppImage 后路径失效，hook 指向一个不存在的文件。

**根因**：`src/AiToolStrategy.h` 的 `resolvedCtlPath()` 用 `QCoreApplication::applicationDirPath()` 拼路径，在 AppImage 里这个目录是挂载点内部的 `/tmp/.mount_traffiXXXXXX/usr/bin`。

**关键约束**：`tl4ai-ctl` 链接了 `Qt6::Core` / `Qt6::Network`，依赖 AppImage 内打包的 Qt 库（靠挂载点 rpath），不能简单拷到 `~/.local/bin` 当独立程序用。

## 最终可行方案

稳定路径的唯一思路是**通过 AppImage 本体**调用——AppImage 运行时会设环境变量 `APPIMAGE`＝那个 `.AppImage` 文件的绝对路径，这个路径稳定。三处配合：

### 1. GUI 二进制加 CLI 转发分支（`src/main.cpp`）

AppImage 默认 AppRun 本来就是去 `exec` GUI 主二进制 `trafficlight4ai`，所以给主二进制加一个 CLI 短路分支即可，`foo.AppImage red` 就能工作，**完全不用改 AppImage 打包、不用自定义 AppRun**：

```cpp
int main(int argc, char *argv[])
{
    // Lightweight CLI-forwarding mode (AppImage hooks: `foo.AppImage red`).
    if (argc >= 2 && Tl4aiClient::isStateCommand(QString::fromLocal8Bit(argv[1]))) {
        Tl4aiClient::drainStdin();
        QCoreApplication app(argc, argv);
        app.setApplicationName("trafficlight4ai");
        return Tl4aiClient::sendState(QString::fromLocal8Bit(argv[1]));
    }

    QApplication app(argc, argv);
    // ... 正常 GUI 流程
}
```

### 2. `resolvedCtlPath()` 感知 AppImage（`src/AiToolStrategy.h`）

拆成可测的 `resolvedCtlPathForDir(dir, appImagePath)`，优先用 `$APPIMAGE` 稳定路径；`/.mount_` 临时目录不取内部 ctl；带空格路径用 JSON 转义：

```cpp
static QString resolvedCtlPathForDir(const QString &dir,
                                     const QString &appImagePath = QString())
{
#ifndef Q_OS_WIN
    if (!appImagePath.isEmpty() && QFile::exists(appImagePath))
        return quoteCommandPath(QFileInfo(appImagePath).absoluteFilePath());
#else
    Q_UNUSED(appImagePath)
#endif
    if (isTransientAppImageDir(dir))   // 含 "/.mount_" 则不用内部 ctl
        return QString("tl4ai-ctl");
    // ... 否则用程序目录旁的 tl4ai-ctl，找不到回退裸名
}

static QString resolvedCtlPath()
{
    const QString dir = QCoreApplication::applicationDirPath();
    return resolvedCtlPathForDir(dir, QString::fromLocal8Bit(qgetenv("APPIMAGE")));
}
```

### 3. DRY：抽出共享 IPC 客户端（`src/Tl4aiClient.h/.cpp`）

把 socket 路径解析、stdin drain、发送逻辑抽到 `tl4ai_core` 的 `Tl4aiClient` 命名空间，`tl4ai-ctl`（`tools/tl4ai_ctl.cpp`）和 GUI 的 CLI 分支共用，避免第三份拷贝。

### 效果

推荐配置不再显示临时路径，而是稳定的：

```
/path/to/trafficlight4ai-<version>-linux-amd64.AppImage red
```

跨重启可用。已在本地编译 + `ctest` 验证通过。

## 注意事项

- **服务端与客户端默认 socket 路径必须一致**：`ConfigManager::defaultSocketPath`（服务端，`src/ConfigManager.cpp:17`）与 `Tl4aiClient::defaultSocketPath`（客户端）都按 `$XDG_RUNTIME_DIR/trafficlight4ai.sock` → macOS `$TMPDIR/...` → `/tmp/trafficlight4ai-<uid>.sock` 解析。两者不一致则 hook 连不上 GUI server。
- **模板是 JSON，带空格路径要用 JSON 转义**：包内引号须用 `\"...\"`（C++ 源码里写成 `QStringLiteral("\\\"")`），不能用裸 `"`，否则生成非法 JSON——既影响复制对话框，也会让"编辑 Hooks 配置"保存时 `QJsonDocument::fromJson` 直接报错。
- **AppImage 测试加 `#ifndef Q_OS_WIN` 守卫**：AppImage 是 Linux 概念，否则 Windows CI 上 `$APPIMAGE` 路径相关断言会失败。
- **CLI 分支必须在构造 `QApplication` 之前**：用 `QCoreApplication` 即可，`QLocalSocket::waitForConnected` 不需要运行事件循环。
- **`tl4ai-ctl` 链接 `tl4ai_core`** 才能拿到 `Tl4aiClient`（`tools/CMakeLists.txt`）。

## 踩过的坑

1. **设想加自定义 AppRun 分发指令** —— 没必要。AppImage 默认 AppRun 本就 `exec` GUI 主二进制，给主二进制加 CLI 分支更简单，且不动打包脚本。
2. **原 `'"' + path + '"'` 引号方式** —— 对带空格路径生成非法 JSON：复制出去是坏的，编辑对话框保存时解析直接报错。改用 `\"` 修复（属于顺带修掉的潜在 bug）。
3. **打算在 `main.cpp` 抄第三份 socket/IPC 逻辑** —— 违反 DRY（`tools/tl4ai_ctl.cpp` 和 `src/ConfigManager.cpp` 已各有一份），抽到 `tl4ai_core` 共享。
4. **想把 `tl4ai-ctl` 单独拷到 `~/.local/bin`** —— 不行，它链接 Qt，离开 AppImage 包就找不到 Qt 库。
5. **想在 `tests/CMakeLists.txt` 里 `add_dependencies(test_tl4ai_ctl tl4ai-ctl)`** —— 会配置失败：`add_subdirectory` 顺序是 src→tests→tools，处理 tests 时 `tl4ai-ctl` 目标尚未定义。沿用现有"全量构建 + QSKIP"模式。

## 参考

- `src/AiToolStrategy.h:213` —— `resolvedCtlPathForDir` / `resolvedCtlPath` / `quoteCommandPath` / `isTransientAppImageDir`
- `src/main.cpp:45` —— CLI 转发短路分支
- `src/Tl4aiClient.h` / `src/Tl4aiClient.cpp` —— 共享 IPC 客户端（在 `tl4ai_core`）
- `tools/tl4ai_ctl.cpp` —— 重构后委托 `Tl4aiClient`
- `src/ConfigManager.cpp:17` —— 服务端默认 socket 路径（需与客户端一致）
- `src/SettingsDialog.cpp:546` —— "查看推荐 Hooks 配置"原样 `setPlainText(resolvedTemplate(...))`
- `tests/test_ai_tool_strategy.cpp` —— AppImage 路径解析测试（`#ifndef Q_OS_WIN`）
- `tests/test_tl4ai_ctl.cpp` —— `appExecutableCanSendCommand` 验证 GUI 二进制也能发 IPC
- `docs/HOOKS.md` / `docs/HOOKS_zh.md` —— AppImage hook 路径说明
- 分支 `fix/appimage-hooks-stable-path`，commit `472c32e`
