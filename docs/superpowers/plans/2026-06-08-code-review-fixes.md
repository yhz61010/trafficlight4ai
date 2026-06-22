# 代码审查修复实施计划

> **致 agentic worker：** 必需的子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 逐任务实施本计划。各步骤使用复选框（`- [ ]`）语法进行追踪。

**目标：** 修复全代码库代码审查中发现的全部 17 个问题——安全、Bug、缺失的 include、效率以及测试覆盖缺口。

**架构：** 修复按关注领域归并为 6 个任务。每个任务均可独立提交。除新增测试外不创建任何新文件。

**技术栈：** C++17、Qt 6、CMake、QTest

---

### 任务 1：安全——IPC socket 访问控制与读取大小限制

**文件：**
- 修改：`src/IpcServer.cpp:59-70`（构造函数）、`src/IpcServer.cpp:84-110`（restart）、`src/IpcServer.cpp:117-138`（onNewConnection）

- [ ] **步骤 1：在构造函数中 listen() 之前添加 UserAccessOption**

在 `src/IpcServer.cpp` 中，修改构造函数，在 listen 之前设置 socket 选项：

```cpp
IpcServer::IpcServer(StateManager *stateManager, const QString &socketPath, QObject *parent)
    : QObject(parent), m_server(std::make_unique<QLocalServer>()),
      m_stateManager(stateManager), m_socketPath(socketPath)
{
    removeStaleSocket(m_socketPath);

    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connectServer();
    m_ownsSocket = m_server->listen(m_socketPath);
    if (!m_ownsSocket)
        qWarning("IpcServer: failed to listen on %s: %s",
                 qPrintable(m_socketPath), qPrintable(m_server->errorString()));
}
```

- [ ] **步骤 2：在 restart() 中 listen() 之前添加 UserAccessOption**

在 `src/IpcServer.cpp` 的 `restart()` 中，在 newServer 上于 listen 之前添加 socket 选项：

```cpp
auto newServer = std::make_unique<QLocalServer>();
newServer->setSocketOptions(QLocalServer::UserAccessOption);

removeStaleSocket(newPath);
if (!newServer->listen(newPath)) {
```

- [ ] **步骤 3：在 onNewConnection 中用 read(64) 替换 readAll()**

在 `src/IpcServer.cpp` 的 `onNewConnection()` 中，修改 lambda：

```cpp
auto processData = [this, client]() {
    QByteArray data = client->read(64);
    if (!data.isEmpty())
        m_stateManager->handleCommand(QString::fromUtf8(data));
    client->disconnectFromServer();
    client->deleteLater();
};
```

- [ ] **步骤 4：提交**

```bash
git add src/IpcServer.cpp
git commit -m "fix: restrict IPC socket to owner-only access and cap read size"
```

---

### 任务 2：Bug——IPC 双重 delete、restart 竞态、缺失 "small" 尺寸、SoundUtils 泄漏

**文件：**
- 修改：`src/IpcServer.cpp:117-138`（onNewConnection）、`src/IpcServer.cpp:84-110`（restart）
- 修改：`src/main.cpp:62-70`
- 修改：`src/SoundUtils.cpp:9-42`

- [ ] **步骤 1：修复 onNewConnection 中的双重 delete——将 disconnected 信号移入 else 分支**

在 `src/IpcServer.cpp` 中，重构 `onNewConnection()`：

```cpp
void IpcServer::onNewConnection()
{
    while (QLocalSocket *client = m_server->nextPendingConnection()) {
        auto processData = [this, client]() {
            QByteArray data = client->read(64);
            if (!data.isEmpty())
                m_stateManager->handleCommand(QString::fromUtf8(data));
            client->disconnectFromServer();
            client->deleteLater();
        };

        if (client->bytesAvailable()) {
            processData();
        } else {
            connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
            connect(client, &QLocalSocket::readyRead, this, processData);
        }
    }
}
```

- [ ] **步骤 2：修复 restart() 竞态——在 listen() 之前连接 newConnection**

在 `src/IpcServer.cpp` 中，重构 `restart()`：

```cpp
bool IpcServer::restart(const QString &newPath)
{
    if (newPath == m_socketPath)
        return isListening();

    auto newServer = std::make_unique<QLocalServer>();
    newServer->setSocketOptions(QLocalServer::UserAccessOption);

    removeStaleSocket(newPath);
    if (!newServer->listen(newPath)) {
        qWarning("IpcServer: failed to listen on %s: %s",
                 qPrintable(newPath), qPrintable(newServer->errorString()));
        return false;
    }

    const QString oldPath = m_socketPath;
    const bool oldOwned = m_ownsSocket;

    m_server->close();
    if (oldOwned)
        removeOwnedServer(oldPath);

    m_server = std::move(newServer);
    m_socketPath = newPath;
    m_ownsSocket = true;
    connectServer();
    return true;
}
```

注意：`connectServer()` 在 `m_server` 被交换之后调用，它会在新服务器上连接 `newConnection`。由于此前已调用 `listen()`，任何在 `listen()` 与 `connectServer()` 之间到达的连接都会在 `nextPendingConnection` 中排队。一旦 `connectServer()` 触发，`onNewConnection` 就会清空该队列。这是安全的，因为信号是排队的，且在同一函数内 `listen()` 与 `connectServer()` 之间不会运行事件循环。

- [ ] **步骤 3：在 main.cpp 中补上缺失的 "small" 分支**

在 `src/main.cpp` 中，于第 64 行之后补上缺失的分支：

```cpp
const QString size = config.windowSize();
if (size == "xsmall")
    lightWidget->setSizePreset(TrafficLightWidget::ExtraSmall);
else if (size == "small")
    lightWidget->setSizePreset(TrafficLightWidget::Small);
else if (size == "medium")
    lightWidget->setSizePreset(TrafficLightWidget::Medium);
else if (size == "large")
    lightWidget->setSizePreset(TrafficLightWidget::Large);
else if (size == "xlarge")
    lightWidget->setSizePreset(TrafficLightWidget::ExtraLarge);
```

- [ ] **步骤 4：修复 SoundUtils 泄漏——将 audioOutput 的父对象设为 player**

在 `src/SoundUtils.cpp` 中，将 `audioOutput` 的父对象设为 `player`，并移除手动的 `audioOutput->deleteLater()` 调用：

```cpp
void playSound(const QString &filePath, QObject *errorContext)
{
    if (!filePath.isEmpty() && QFile::exists(filePath)) {
        auto *player = new QMediaPlayer();
        auto *audioOutput = new QAudioOutput(player);
        player->setAudioOutput(audioOutput);
        player->setSource(QUrl::fromLocalFile(filePath));

        QObject::connect(player, &QMediaPlayer::errorOccurred,
                         player, [player, errorContext, filePath]
                         (QMediaPlayer::Error, const QString &) {
            if (errorContext) {
                auto *widget = qobject_cast<QWidget *>(errorContext);
                QMessageBox::warning(widget,
                    QObject::tr("Audio Error"),
                    QObject::tr("Invalid audio file: %1").arg(filePath));
            }
            player->deleteLater();
        });

        QObject::connect(player, &QMediaPlayer::playbackStateChanged,
                         player, [player](QMediaPlayer::PlaybackState state) {
            if (state == QMediaPlayer::StoppedState) {
                player->deleteLater();
            }
        });

        player->play();
    } else {
        QApplication::beep();
    }
}
```

- [ ] **步骤 5：提交**

```bash
git add src/IpcServer.cpp src/main.cpp src/SoundUtils.cpp
git commit -m "fix: IPC double-delete, restart race, missing small size, SoundUtils leak"
```

---

### 任务 3：缺失的显式 #include（CLAUDE.md 合规）

**文件：**
- 修改：`src/TrayIcon.h`
- 修改：`src/IpcServer.h`
- 修改：`src/TrafficLightWidget.cpp`
- 修改：`tests/test_ai_tool_strategy.cpp`
- 修改：`tests/test_tl4ai_ctl.cpp`

- [ ] **步骤 1：在 TrayIcon.h 中添加 QColor 和 QIcon**

在 `#include <QSystemTrayIcon>` 之后添加：

```cpp
#include <QColor>
#include <QIcon>
```

- [ ] **步骤 2：在 IpcServer.h 中添加 QString**

在 `#include <QObject>` 之后添加：

```cpp
#include <QString>
```

- [ ] **步骤 3：在 TrafficLightWidget.cpp 中添加 QEasingCurve**

在 `#include <QPainter>` 之后添加：

```cpp
#include <QEasingCurve>
```

- [ ] **步骤 4：在 test_ai_tool_strategy.cpp 中添加 QRegularExpression**

在 `#include <QtTest>` 之后添加：

```cpp
#include <QRegularExpression>
```

- [ ] **步骤 5：在 test_tl4ai_ctl.cpp 中添加 QTemporaryDir 和 QFile**

在 `#include <QProcess>` 之后添加：

```cpp
#include <QTemporaryDir>
#include <QFile>
```

- [ ] **步骤 6：提交**

```bash
git add src/TrayIcon.h src/IpcServer.h src/TrafficLightWidget.cpp tests/test_ai_tool_strategy.cpp tests/test_tl4ai_ctl.cpp
git commit -m "fix: add missing explicit Qt includes per CLAUDE.md rules"
```

---

### 任务 4：效率——TrayIcon pixmap 复用与 ConfigManager 批量保存

**文件：**
- 修改：`src/TrayIcon.h`
- 修改：`src/TrayIcon.cpp`
- 修改：`src/ConfigManager.h`
- 修改：`src/ConfigManager.cpp`

- [ ] **步骤 1：在 TrayIcon 中复用 QPixmap，而非每帧分配**

在 `src/TrayIcon.h` 中，添加一个成员：

```cpp
QPixmap m_iconPixmap{64, 64};
```

在 `src/TrayIcon.cpp` 中，修改 `createIcon()` 以复用该 pixmap：

```cpp
QIcon TrayIcon::createIcon(const QColor &color) const
{
    m_iconPixmap.fill(Qt::transparent);

    QPainter painter(&m_iconPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(4, 4, 64 - 8, 64 - 8);

    return QIcon(m_iconPixmap);
}
```

由于 `m_iconPixmap` 是在 `const` 方法中使用的可变状态，应将其声明为 `mutable`：

```cpp
mutable QPixmap m_iconPixmap{64, 64};
```

- [ ] **步骤 2：为 ConfigManager 添加 beginBatchSave/endBatchSave**

在 `src/ConfigManager.h` 中，添加：

```cpp
void beginBatchSave();
void endBatchSave();
```

以及一个私有成员：

```cpp
bool m_batchSave = false;
```

在 `src/ConfigManager.cpp` 中，实现：

```cpp
void ConfigManager::beginBatchSave()
{
    m_batchSave = true;
}

void ConfigManager::endBatchSave()
{
    m_batchSave = false;
    save();
}
```

并在每个 setter 中守护 `save()` 调用。将现有 `save()` 方法的方法体改为：

在每个 setter 的末尾（即私有的 `save()` 调用处），将其包裹起来：

实际上，更干净的做法是——修改 `save()` 本身，使其检查该标志：

```cpp
void ConfigManager::save()
{
    if (m_batchSave)
        return;

    QDir dir = QFileInfo(m_configPath).absoluteDir();
    if (!dir.exists())
        dir.mkpath(".");

    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
        file.close();
    }
}
```

- [ ] **步骤 3：在 SettingsDialog::restoreSnapshot() 中使用批量保存**

在 `src/SettingsDialog.cpp` 中，包裹 `restoreSnapshot()` 的 setter 调用：

```cpp
void SettingsDialog::restoreSnapshot()
{
    m_config->beginBatchSave();

    // ... all existing restore code unchanged ...

    m_config->endBatchSave();
}
```

- [ ] **步骤 4：提交**

```bash
git add src/TrayIcon.h src/TrayIcon.cpp src/ConfigManager.h src/ConfigManager.cpp src/SettingsDialog.cpp
git commit -m "perf: reuse TrayIcon pixmap and batch ConfigManager saves"
```

---

### 任务 5：测试覆盖——ConfigManager 语言/声音、Claude Code 事件、运行中调用 StateManager setTimeoutSec(0)

**文件：**
- 修改：`tests/test_config_manager.cpp`
- 修改：`tests/test_ai_tool_strategy.cpp`
- 修改：`tests/test_state_manager.cpp`

- [ ] **步骤 1：在 test_config_manager.cpp 中添加语言和声音测试**

在结尾的 `};` 之前添加这些测试槽：

```cpp
void defaultLanguage()
{
    ConfigManager cm(m_configPath);
    QCOMPARE(cm.language(), QString("en"));
}

void setAndGetLanguage()
{
    ConfigManager cm(m_configPath);
    cm.setLanguage("zh");
    QCOMPARE(cm.language(), QString("zh"));
}

void languagePersists()
{
    {
        ConfigManager cm(m_configPath);
        cm.setLanguage("ja");
    }
    ConfigManager cm2(m_configPath);
    QCOMPARE(cm2.language(), QString("ja"));
}

void defaultYellowSoundEnabled()
{
    ConfigManager cm(m_configPath);
    QCOMPARE(cm.yellowSoundEnabled(), true);
}

void setAndGetYellowSoundEnabled()
{
    ConfigManager cm(m_configPath);
    cm.setYellowSoundEnabled(false);
    QCOMPARE(cm.yellowSoundEnabled(), false);
}

void defaultGreenSoundEnabled()
{
    ConfigManager cm(m_configPath);
    QCOMPARE(cm.greenSoundEnabled(), true);
}

void setAndGetGreenSoundEnabled()
{
    ConfigManager cm(m_configPath);
    cm.setGreenSoundEnabled(false);
    QCOMPARE(cm.greenSoundEnabled(), false);
}

void defaultYellowSoundFile()
{
    ConfigManager cm(m_configPath);
    QCOMPARE(cm.yellowSoundFile(), QString());
}

void setAndGetYellowSoundFile()
{
    ConfigManager cm(m_configPath);
    cm.setYellowSoundFile("/tmp/alert.wav");
    QCOMPARE(cm.yellowSoundFile(), QString("/tmp/alert.wav"));
}

void defaultGreenSoundFile()
{
    ConfigManager cm(m_configPath);
    QCOMPARE(cm.greenSoundFile(), QString());
}

void setAndGetGreenSoundFile()
{
    ConfigManager cm(m_configPath);
    cm.setGreenSoundFile("/tmp/done.ogg");
    QCOMPARE(cm.greenSoundFile(), QString("/tmp/done.ogg"));
}

void soundSettingsPersist()
{
    {
        ConfigManager cm(m_configPath);
        cm.setYellowSoundEnabled(false);
        cm.setYellowSoundFile("/tmp/y.wav");
        cm.setGreenSoundEnabled(false);
        cm.setGreenSoundFile("/tmp/g.mp3");
    }
    ConfigManager cm2(m_configPath);
    QCOMPARE(cm2.yellowSoundEnabled(), false);
    QCOMPARE(cm2.yellowSoundFile(), QString("/tmp/y.wav"));
    QCOMPARE(cm2.greenSoundEnabled(), false);
    QCOMPARE(cm2.greenSoundFile(), QString("/tmp/g.mp3"));
}
```

- [ ] **步骤 2：在 test_ai_tool_strategy.cpp 中添加 Claude Code 事件校验测试**

添加这个测试槽：

```cpp
void claudeTemplateOnlyUsesValidEvents()
{
    const QStringList claudeEvents = {
        "PreToolUse", "PostToolUse", "Notification", "Stop",
        "SubagentStart", "SubagentStop", "UserPromptSubmit",
        "PermissionRequest", "SessionEnd"
    };

    ClaudeCodeStrategy claude;
    const QString tmpl = claude.hooksTemplate();
    QRegularExpression re(R"RE("(\w+)":\s*[\[{])RE");
    auto it = re.globalMatch(tmpl);
    while (it.hasNext()) {
        auto match = it.next();
        QString event = match.captured(1);
        if (event == "hooks" || event == "command")
            continue;
        QVERIFY2(claudeEvents.contains(event),
                  qPrintable("Invalid Claude Code event: " + event));
    }
}
```

- [ ] **步骤 3：在 test_state_manager.cpp 中添加运行中调用 setTimeoutSec(0) 的测试**

添加这个测试槽：

```cpp
void setTimeoutZeroWhileWorkingCancelsTimer()
{
    StateManager sm;
    sm.setTimeoutSec(1);
    sm.setState(LightState::Working);
    // Disable timeout while working
    sm.setTimeoutSec(0);
    QTest::qWait(1500);
    QCOMPARE(sm.state(), LightState::Working); // timer was cancelled
}
```

- [ ] **步骤 4：提交**

```bash
git add tests/test_config_manager.cpp tests/test_ai_tool_strategy.cpp tests/test_state_manager.cpp
git commit -m "test: add coverage for language, sound, Claude Code events, and timeout cancel"
```

---

### 任务 6：去重 sizes/presets 映射

**文件：**
- 修改：`src/TrafficLightWidget.h`
- 修改：`src/TrafficLightWidget.cpp`
- 修改：`src/main.cpp`
- 修改：`src/SettingsDialog.cpp`

- [ ] **步骤 1：为 TrafficLightWidget 添加静态方法 sizePresetFromString()**

在 `src/TrafficLightWidget.h` 中，添加一个 public 静态方法：

```cpp
static SizePreset sizePresetFromString(const QString &size);
```

在 `src/TrafficLightWidget.cpp` 中，实现：

```cpp
TrafficLightWidget::SizePreset TrafficLightWidget::sizePresetFromString(const QString &size)
{
    if (size == "xsmall") return ExtraSmall;
    if (size == "small")  return Small;
    if (size == "medium") return Medium;
    if (size == "large")  return Large;
    if (size == "xlarge") return ExtraLarge;
    return Small; // default
}
```

- [ ] **步骤 2：在 main.cpp 中使用 sizePresetFromString**

替换 `src/main.cpp` 中的 if-else 链：

```cpp
lightWidget->setSizePreset(TrafficLightWidget::sizePresetFromString(config.windowSize()));
```

- [ ] **步骤 3：在 SettingsDialog::onWindowSizeChanged 中使用 sizePresetFromString**

在 `src/SettingsDialog.cpp` 中，简化 `onWindowSizeChanged()`：

```cpp
void SettingsDialog::onWindowSizeChanged(int index)
{
    const QStringList sizes = {"xsmall", "small", "medium", "large", "xlarge"};
    if (index < 0 || index >= sizes.size())
        return;

    QWidget *window = m_lightWidget->window();
    QPoint pos = window->pos();

    m_config->setWindowSize(sizes.at(index));
    m_lightWidget->setSizePreset(TrafficLightWidget::sizePresetFromString(sizes.at(index)));

    resizeFloatingWindowAt(pos, true);
}
```

- [ ] **步骤 4：在 SettingsDialog::restoreSnapshot 中使用 sizePresetFromString**

在 `src/SettingsDialog.cpp` 中，简化 `restoreSnapshot()` 中的尺寸恢复代码块：

```cpp
// Restore window size (preserve position)
QWidget *window = m_lightWidget->window();
QPoint pos = window->pos();
m_config->setWindowSize(m_snapSize);
m_lightWidget->setSizePreset(TrafficLightWidget::sizePresetFromString(m_snapSize));
resizeFloatingWindowAt(pos, false);
```

- [ ] **步骤 5：提交**

```bash
git add src/TrafficLightWidget.h src/TrafficLightWidget.cpp src/main.cpp src/SettingsDialog.cpp
git commit -m "refactor: deduplicate size preset mapping with sizePresetFromString()"
```
