#pragma once

#include <QString>
#include <QDir>
#include <QList>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QtGlobal>
#include <memory>

class AiToolStrategy {
public:
    virtual ~AiToolStrategy() = default;
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual int defaultTimeoutSec() const = 0;
    virtual QString hooksTemplate() const = 0;
    virtual QString hooksConfigPath() const = 0;
    virtual bool hooksIsEntireFile() const = 0;
};

class CodexStrategy : public AiToolStrategy {
public:
    QString id() const override { return "codex"; }
    QString displayName() const override { return "Codex"; }
    int defaultTimeoutSec() const override { return 300; }
    QString hooksTemplate() const override
    {
        return R"({
  "hooks": {
    "UserPromptSubmit": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }] }
    ],
    "PreToolUse": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }] }
    ],
    "SubagentStart": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }] }
    ],
    "PermissionRequest": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl yellow" }] }
    ],
    "Stop": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl green" }] }
    ]
  }
})";
    }
    QString hooksConfigPath() const override { return QDir::homePath() + "/.codex/hooks.json"; }
    bool hooksIsEntireFile() const override { return true; }
};

class ClaudeCodeStrategy : public AiToolStrategy {
public:
    QString id() const override { return "claude-code"; }
    QString displayName() const override { return "Claude Code"; }
    int defaultTimeoutSec() const override { return 300; }
    QString hooksTemplate() const override
    {
        return R"({
  "hooks": {
    "UserPromptSubmit": [{ "command": "tl4ai-ctl red" }],
    "PreToolUse": [{ "command": "tl4ai-ctl red" }],
    "SubagentStart": [{ "command": "tl4ai-ctl red" }],
    "Notification": [{ "command": "tl4ai-ctl yellow" }],
    "PermissionRequest": [{ "command": "tl4ai-ctl yellow" }],
    "Stop": [{ "command": "tl4ai-ctl green" }],
    "SessionEnd": [{ "command": "tl4ai-ctl green" }]
  }
})";
    }
    QString hooksConfigPath() const override { return QDir::homePath() + "/.claude/settings.json"; }
    bool hooksIsEntireFile() const override { return false; }
};

class QoderCnStrategy : public AiToolStrategy {
public:
    QString id() const override { return "qoder-cn"; }
    QString displayName() const override { return "Qoder CN"; }
    int defaultTimeoutSec() const override { return 300; }
    QString hooksTemplate() const override
    {
        return R"({
  "hooks": {
    "UserPromptSubmit": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }] }
    ],
    "PreToolUse": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }] }
    ],
    "SubagentStart": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }] }
    ],
    "Notification": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl yellow" }] }
    ],
    "PermissionRequest": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl yellow" }] }
    ],
    "Stop": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl green" }] }
    ],
    "SessionEnd": [
      { "hooks": [{ "type": "command", "command": "tl4ai-ctl green" }] }
    ]
  }
})";
    }
    QString hooksConfigPath() const override { return QDir::homePath() + "/.qoder-cn/settings.json"; }
    bool hooksIsEntireFile() const override { return false; }
};

class CopilotStrategy : public AiToolStrategy {
public:
    QString id() const override { return "copilot"; }
    QString displayName() const override { return "Copilot"; }
    int defaultTimeoutSec() const override { return 300; }
    QString hooksTemplate() const override
    {
        return R"({
  "version": 1,
  "hooks": {
    "userPromptSubmitted": [
      { "type": "command", "command": "tl4ai-ctl red" }
    ],
    "preToolUse": [
      { "type": "command", "command": "tl4ai-ctl red" }
    ],
    "subagentStart": [
      { "type": "command", "command": "tl4ai-ctl red" }
    ],
    "notification": [
      { "type": "command", "command": "tl4ai-ctl yellow" }
    ],
    "permissionRequest": [
      { "type": "command", "command": "tl4ai-ctl yellow" }
    ],
    "agentStop": [
      { "type": "command", "command": "tl4ai-ctl green" }
    ],
    "sessionEnd": [
      { "type": "command", "command": "tl4ai-ctl green" }
    ]
  }
})";
    }
    QString hooksConfigPath() const override { return QDir::homePath() + "/.copilot/hooks/trafficlight4ai.json"; }
    bool hooksIsEntireFile() const override { return true; }
};

class GeminiStrategy : public AiToolStrategy {
public:
    QString id() const override { return "gemini"; }
    QString displayName() const override { return "Gemini"; }
    int defaultTimeoutSec() const override { return 300; }
    QString hooksTemplate() const override
    {
        return R"({
  "hooks": {
    "BeforeAgent": [
      {
        "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }]
      }
    ],
    "BeforeTool": [
      {
        "hooks": [{ "type": "command", "command": "tl4ai-ctl red" }]
      }
    ],
    "Notification": [
      {
        "hooks": [{ "type": "command", "command": "tl4ai-ctl yellow" }]
      }
    ],
    "AfterAgent": [
      {
        "hooks": [{ "type": "command", "command": "tl4ai-ctl green" }]
      }
    ],
    "SessionEnd": [
      {
        "hooks": [{ "type": "command", "command": "tl4ai-ctl green" }]
      }
    ]
  }
})";
    }
    QString hooksConfigPath() const override { return QDir::homePath() + "/.gemini/settings.json"; }
    bool hooksIsEntireFile() const override { return false; }
};

class AiToolRegistry {
public:
    static const QList<AiToolStrategy *> &strategies()
    {
        static CodexStrategy codex;
        static ClaudeCodeStrategy claude;
        static QoderCnStrategy qoderCn;
        static CopilotStrategy copilot;
        static GeminiStrategy gemini;
        static QList<AiToolStrategy *> list = {&codex, &claude, &qoderCn, &copilot, &gemini};
        return list;
    }

    static AiToolStrategy *find(const QString &id)
    {
        for (auto *s : strategies()) {
            if (s->id() == id)
                return s;
        }
        return nullptr;
    }

    // Quote a path for embedding inside a JSON string value. The templates are
    // JSON, so a space-containing path must be wrapped in JSON-escaped quotes
    // (\"...\") to stay valid both when displayed and when parsed/re-serialized.
    static QString quoteCommandPath(const QString &path)
    {
        return path.contains(' ')
            ? (QStringLiteral("\\\"") + path + QStringLiteral("\\\""))
            : path;
    }

    // AppImage mounts the bundled tl4ai-ctl under a transient /tmp/.mount_* path
    // that changes every run, so it must never be written into a saved hook.
    static bool isTransientAppImageDir(const QString &dir)
    {
#ifdef Q_OS_WIN
        Q_UNUSED(dir)
        return false;
#else
        const QString cleanDir = QDir::cleanPath(dir);
        return cleanDir.contains(QLatin1String("/.mount_"));
#endif
    }

    // Resolve the command path for hook templates. Split out for testability.
    // Priority: stable .AppImage path (when running as AppImage) > tl4ai-ctl
    // next to the executable > bare `tl4ai-ctl` (rely on PATH).
    static QString resolvedCtlPathForDir(const QString &dir,
                                         const QString &appImagePath = QString())
    {
#ifndef Q_OS_WIN
        if (!appImagePath.isEmpty() && QFile::exists(appImagePath))
            return quoteCommandPath(QFileInfo(appImagePath).absoluteFilePath());
#else
        Q_UNUSED(appImagePath)
#endif
        if (isTransientAppImageDir(dir))
            return QString("tl4ai-ctl");
#ifdef Q_OS_WIN
        const QString path = dir + "/tl4ai-ctl.exe";
#else
        const QString path = dir + "/tl4ai-ctl";
#endif
        if (!QFile::exists(path))
            return QString("tl4ai-ctl");
        return quoteCommandPath(path);
    }

    static QString resolvedCtlPath()
    {
        const QString dir = QCoreApplication::applicationDirPath();
        return resolvedCtlPathForDir(dir, QString::fromLocal8Bit(qgetenv("APPIMAGE")));
    }

    static QString resolvedTemplate(const AiToolStrategy *strategy)
    {
        return strategy->hooksTemplate().replace(
            QLatin1String("tl4ai-ctl"), resolvedCtlPath());
    }
};
