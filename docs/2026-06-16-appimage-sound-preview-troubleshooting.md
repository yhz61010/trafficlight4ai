# AppImage 声音预览（设置页"试听"按钮）排障记录

> 日期：2026-06-16

## 背景 / 问题现象

AppImage 安装包在 Linux 桌面运行后，进入设置页面点击声音的**"试听"按钮**，出现两个阶段的问题：

- **阶段一（闪退）**：点击"试听"后程序直接崩溃退出。
- **阶段二（不闪退但无声 / 卡死）**：第一轮修复（兜底 + 捆绑多媒体插件）后程序不再闪退，但点击"试听"后**无法播放默认音效**，界面卡死，进程最终被杀。

其它打包形式（deb / rpm / Arch 等，依赖系统自带 Qt 与 GStreamer）无此问题，**仅 AppImage 复现**——这是定位方向的关键线索：问题出在 AppImage 自带的运行时上，而非应用逻辑。

## 调查过程

### 1. 先想清楚要抓什么

阶段二是"卡死"而不是干净退出，光看日志不够，**堆栈（backtrace）才能定位卡在哪一行**。因此同时抓两样东西：终端日志 + 卡死瞬间的 gdb 堆栈。

### 2. 如何通过日志排查

从终端运行 AppImage，标准输出/错误就是日志，再叠加几个诊断环境变量：

```bash
cd /AppImage/所在目录
chmod +x trafficlight4ai-*-linux-amd64.AppImage

QT_DEBUG_PLUGINS=1 \
QT_LOGGING_RULES='qt.multimedia.*=true' \
GST_DEBUG=3 \
./trafficlight4ai-*-linux-amd64.AppImage 2>&1 | tee /tmp/tl4ai.log
```

各变量含义：

| 变量 | 作用 |
|------|------|
| `QT_DEBUG_PLUGINS=1` | 打印 Qt 加载了哪些插件、哪个加载失败（能看出多媒体后端插件是否被找到） |
| `QT_LOGGING_RULES='qt.multimedia.*=true'` | 打开 Qt 多媒体子系统日志 |
| `GST_DEBUG=3` | GStreamer 诊断日志（声音解码链路都在这里） |
| `2>&1 \| tee /tmp/tl4ai.log` | 同时显示并存盘，便于分享 |

> 若运行时报 FUSE 挂载失败，在最前面加 `APPIMAGE_EXTRACT_AND_RUN=1`。

### 3. 卡死瞬间抓堆栈

程序卡死时**先别关**，另开终端：

```bash
which gdb || sudo apt install -y gdb
pid=$(pgrep -x trafficlight4ai)
gdb -p "$pid" -batch -ex 'thread apply all bt' > /tmp/tl4ai-bt.txt 2>&1
```

### 4. 日志暴露的关键信息

`GST_DEBUG=3` 的输出里出现了大量插件加载失败，归纳后定位到三类证据：

```text
# 证据 A：宿主插件版本不兼容（宿主 1.26 插件 vs 自带 1.20 核心）
plugin "libgstapp.so" has incompatible version (plugin: 1.26, gst: 1.20), not loading
module_open failed: .../libgstvideoconvertscale.so: undefined symbol: gst_caps_features_get_nth_id_str

# 证据 B：连自带插件也加载失败（glib 混搭）
Failed to load plugin '/tmp/.mount_xxx/usr/lib/gstreamer-1.0/libgsttypefindfunctions.so':
  /lib/x86_64-linux-gnu/libgio-2.0.so.0: undefined symbol: g_assertion_message_cmpint

# 证据 C：拿不到必需元件，管线建不起来
no such element factory "videoconvert"!
no such element factory "volume"!
no such element factory "autoaudiosink"!
GStreamer-CRITICAL: gst_element_get_static_pad: assertion 'GST_IS_ELEMENT (element)' failed
[appimagelauncher-binfmt-bypass/lib] ERROR: child exited with code 9
```

`QT_DEBUG_PLUGINS=1` 还显示出 Qt 版本号 `393728`（即 Qt 6.2），结合宿主插件版本号可确认：**构建机是低版本系统（GStreamer 1.20 / Qt 6.2，对应 Ubuntu 22.04），用户机是 GStreamer 1.26**。

## 根因分析

崩溃/卡死由**两个独立病根叠加**导致：

**病根①：宿主插件污染。** 自带的 GStreamer 初始化只对 `GST_PLUGIN_PATH` 做了 *prepend*，没有禁用宿主插件目录。于是 GStreamer 同时扫描"自带 1.20 插件"和"宿主 1.26 插件"。宿主 1.26 插件被塞进 1.20 核心加载，触发 `incompatible version` 和一堆 `undefined symbol`（证据 A）。

**病根②：glib 版本混搭。** 旧的 `libglib-2.0` 被捆进 AppImage，但 `libgio-2.0` 没被捆（当时没有任何自带插件依赖它），于是自带插件运行时拉到的是**宿主的 libgio（新）+ 自带的 libglib（旧）**，符号对不上（证据 B）。

最终结果：一个可用的音频元件都凑不齐（`volume` / `autoaudiosink` / `videoconvert` 全缺），Qt 建不出音频管线 → GStreamer 抛 CRITICAL 断言 → 主线程卡死 → 进程被杀（证据 C）。

> 这也解释了为什么阶段一加的 beep 兜底没生效：崩溃发生在 GStreamer 的 CRITICAL / 建管线阶段，**根本没走到 Qt 的 `errorOccurred` 信号**，兜底逻辑被绕过。

## 最终可行方案

方案核心：**把自带 GStreamer 与宿主彻底隔离，并补全成一套自洽的运行时**。

### 1. 运行时隔离（`src/SoundUtils.cpp` 的 `initBundledGStreamerPlugins()`）

```cpp
// 只指向自带插件目录，并禁用宿主系统插件扫描
setEnvPath("GST_PLUGIN_PATH", encodedPluginDir);
setEnvPath("GST_PLUGIN_PATH_1_0", encodedPluginDir);
qputenv("GST_PLUGIN_SYSTEM_PATH_1_0", QByteArray());   // 关键：空值 = 不扫描宿主插件
qputenv("GST_PLUGIN_SCANNER", <自带 scanner>);          // 用自带扫描器
qputenv("GST_REGISTRY_1_0", <私有 registry 路径>);      // 不复用宿主缓存
```

该函数在 `src/main.cpp` 中 `QApplication` 构造后、任何多媒体使用前调用，确保在 GStreamer 注册表扫描之前生效。

### 2. 打包侧补全（`packaging/linux/build-appimage.sh`）

- **补全插件清单**：在原有 `coreelements / typefindfunctions / playback / ogg / vorbis / audioconvert / audioresample` 之外，补上 `volume`、`autodetect`（autoaudiosink）、`app`（appsrc）、`videoconvert`、`gio`、`pulseaudio`、`alsa`。其中 `libgstgio` 会通过依赖把 `libgio-2.0` 一并捆进来，**让 glib 这套库变成一致来源**（解决病根②）。
- **捆绑 `gst-plugin-scanner`** 到 `usr/libexec/gstreamer-1.0/`。
- **RPATH 手术**：给自带插件打 `$ORIGIN:$ORIGIN/..`、给 scanner 打 `$ORIGIN/../../lib:$ORIGIN`，保证它们加载**自带的** libgst/glib，而不是宿主的。
- **打包前先验证**：逐个 `test -f` 必需插件 + `test -x` scanner + `ldd ... | grep 'not found'`，缺任何一项立即 `FATAL` 退出。

### 3. 插件名按构建机 GStreamer 版本修正

第一轮打包 CI 报错：

```text
FATAL: missing required GStreamer plugin libgstvideoconvertscale.so
```

原因：`videoconvert` + `videoscale` 合并成 `videoconvertscale` 是 **GStreamer 1.22+** 才有的文件名（对应用户机 1.26），而**构建机是 1.20**，提供 `videoconvert` 元件的文件叫 `libgstvideoconvert.so`。因为我们是**从构建机捆绑、运行时隔离只用自带插件**，所以必须按构建机的文件名来：

```diff
- libgstvideoconvertscale
+ libgstvideoconvert libgstvideoscale
```

捆绑列表与两处校验列表（`build-appimage.sh` 与 `.github/workflows/package-jobs.yml`）同步改名后，CI 全绿，真机点"试听"正常出声。

## 注意事项

- **构建机用低版本系统，但必须隔离**：低版本（1.20）构建兼容更多用户，但只有把宿主插件扫描关掉（`GST_PLUGIN_SYSTEM_PATH_1_0=""`）+ 用自带 scanner + 私有 registry，自带插件才不会和宿主插件相互污染。
- **GStreamer 插件文件名跨版本会变**：捆绑哪些插件、校验哪些插件，文件名要以**构建机的 GStreamer 版本**为准，不能照搬用户机上看到的文件名（典型如 `videoconvert` 1.20 vs `videoconvertscale` 1.22+）。
- **glib 必须一致来源**：要么全自带、要么全宿主，绝不能"捆 libglib 但不捆 libgio"造成半新半旧混搭。捆 `libgstgio` 是把 libgio 拉齐的简便手段。
- **隔离要在多媒体初始化前完成**：`initBundledGStreamerPlugins()` 必须在第一次创建 `QMediaPlayer` 之前调用（放在 `main.cpp` 启动阶段），否则 GStreamer 注册表已扫描完，环境变量改了也来不及。
- **打包脚本自带验证是最后一道闸**：必需插件 / scanner / `ldd not found` 的检查能把"少捆了东西"在 CI 阶段就拦下，不会发出一个静默坏掉的包。

## 踩过的坑

1. **`EXTRA_QT_PLUGINS` 环境变量** —— 当前 linuxdeploy-plugin-qt 已废弃该变量，会打印警告但**静默不部署**多媒体插件，改用手动 `cp -a` 拷贝。
2. **只 prepend `GST_PLUGIN_PATH`** —— 没禁用宿主插件目录，宿主 1.26 插件照样被扫描并污染（病根①）。必须额外置空 `GST_PLUGIN_SYSTEM_PATH_1_0`。
3. **指望 beep 兜底接住崩溃** —— 卡死发生在 GStreamer CRITICAL / 建管线阶段，没走到 Qt `errorOccurred`，兜底被绕过；根因还得在打包侧解决。
4. **只捆 libglib 不捆 libgio** —— 造成 glib 半新半旧混搭，连自带插件都加载失败（病根②）。
5. **插件名照搬用户机（`libgstvideoconvertscale`）** —— 构建机 1.20 上无此文件，校验直接 `FATAL`，应改用 1.20 的 `libgstvideoconvert`。

## 参考

- `src/SoundUtils.cpp` —— `initBundledGStreamerPlugins()` 运行时隔离逻辑
- `src/main.cpp` —— 启动阶段调用隔离初始化
- `packaging/linux/build-appimage.sh` —— 插件捆绑、scanner 捆绑、RPATH 手术、打包前验证
- `.github/workflows/package-jobs.yml` —— AppImage job 的多媒体内容验证步骤
- 相关提交：
  - `ac736e7` fix: isolate bundled GStreamer in AppImage
  - `9be9c3b` fix: bundle appsrc and videoconvert for AppImage audio
  - `9070610` fix: bundle videoconvert plugin under its GStreamer 1.20 name
