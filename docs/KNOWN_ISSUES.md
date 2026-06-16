# 已知平台问题

- **GitHub Actions workflow 推送**：修改 `.github/workflows/` 需要 PAT 包含 `workflow` scope，否则推送被拒绝
- **openSUSE Leap 15.6 CI**：默认 GCC 7 不支持 Qt 6 所需的 C++17，需用 `gcc-13`/`g++-13`
- **Ubuntu SNI 托盘**：快速频繁 `setIcon` 后切换到静态图标可能不刷新，需 `QTimer::singleShot(150, ...)` 延迟重设
- **QSystemTrayIcon 不是 QWidget**：`QMenu` 不能用 `setParent(this)`，需用 `destroyed` signal 管理生命周期
- **FloatingWindow 尺寸切换**：需用 `QTimer::singleShot(0, ...)` 延迟 `move()` 以在布局重算后恢复位置
- **AppImage 构建 CDN 超时**：linuxdeploy/type2-runtime 下载可能因 GitHub CDN 504 失败，`build-appimage.sh` 已实现固定版本+continuous fallback 机制和 SHA256 校验
- **AppImage 声音预览崩溃/无声**：自带 GStreamer 与宿主版本错位（构建机 1.20 vs 用户机 1.26）导致点击"试听"闪退或卡死，需隔离宿主插件（`GST_PLUGIN_SYSTEM_PATH_1_0=""`）并按构建机版本捆全插件，详见 [排障记录](2026-06-16-appimage-sound-preview-troubleshooting.md)
- **QLocalServer::listen() 与普通文件**：Qt 可能在路径被普通文件占用时仍允许 listen 成功，需在调用前用 `pathBlockedByNonSocket()` 显式拒绝
- **macOS socket 路径 $TMPDIR**：`$TMPDIR` fallback 必须限制在 `Q_OS_MACOS` / `__APPLE__` 下，不可在 Linux 上生效（Linux 的 TMPDIR 会导致 GUI 和 CLI socket 路径不一致）
