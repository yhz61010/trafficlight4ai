# 编辑 Hooks 配置 实现计划

> **致 agentic worker：** 必需子技能：使用 superpowers:subagent-driven-development（推荐）或 superpowers:executing-plans 来逐任务实现本计划。各步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 在 SettingsDialog 中添加一个"编辑 Hooks 配置"按钮，用于打开所选 AI 工具的 hooks 配置文件编辑器，并针对不同的文件结构提供智能的读写逻辑。

**架构：** 为 `AiToolStrategy` 扩展两个新的虚方法（`hooksConfigPath()`、`hooksIsEntireFile()`）。在 `SettingsDialog` 中添加一个新按钮和槽函数，用于打开带有 JSON 感知读写逻辑的编辑器对话框。对于将 hooks 嵌入到更大设置文件中的工具，该编辑器会提取/合并 `"hooks"` 字段。

**技术栈：** C++17、Qt 6（Core、Widgets）、QJsonDocument

---

### 任务 1：为 AiToolStrategy 接口扩展 hooksConfigPath() 和 hooksIsEntireFile()

**文件：**
- 修改：`src/AiToolStrategy.h`
- 修改：`tests/test_ai_tool_strategy.cpp`

- [ ] **步骤 1：向 AiToolStrategy 添加纯虚方法并在所有策略中实现**

在 `src/AiToolStrategy.h` 中，在 `hooksTemplate()` 之后为 `AiToolStrategy` 添加两个新的纯虚方法：

```cpp
virtual QString hooksConfigPath() const = 0;
virtual bool hooksIsEntireFile() const = 0;
```

在文件顶部添加 `#include <QDir>`（`QDir::homePath()` 需要）。

在 `CodexStrategy` 中实现：

```cpp
QString hooksConfigPath() const override { return QDir::homePath() + "/.codex/hooks.json"; }
bool hooksIsEntireFile() const override { return true; }
```

在 `ClaudeCodeStrategy` 中实现：

```cpp
QString hooksConfigPath() const override { return QDir::homePath() + "/.claude/settings.json"; }
bool hooksIsEntireFile() const override { return false; }
```

在 `QoderCnStrategy` 中实现：

```cpp
QString hooksConfigPath() const override { return QDir::homePath() + "/.qoder-cn/settings.json"; }
bool hooksIsEntireFile() const override { return false; }
```

- [ ] **步骤 2：为新方法添加测试**

在 `tests/test_ai_tool_strategy.cpp` 中，在 `registryFindsQoderCn()` 之前添加这些测试槽函数：

```cpp
void codexHooksConfigPath()
{
    CodexStrategy codex;
    QVERIFY(codex.hooksConfigPath().endsWith("/.codex/hooks.json"));
}

void codexHooksIsEntireFile()
{
    CodexStrategy codex;
    QCOMPARE(codex.hooksIsEntireFile(), true);
}

void claudeHooksConfigPath()
{
    ClaudeCodeStrategy claude;
    QVERIFY(claude.hooksConfigPath().endsWith("/.claude/settings.json"));
}

void claudeHooksIsNotEntireFile()
{
    ClaudeCodeStrategy claude;
    QCOMPARE(claude.hooksIsEntireFile(), false);
}

void qoderCnHooksConfigPath()
{
    QoderCnStrategy qoderCn;
    QVERIFY(qoderCn.hooksConfigPath().endsWith("/.qoder-cn/settings.json"));
}

void qoderCnHooksIsNotEntireFile()
{
    QoderCnStrategy qoderCn;
    QCOMPARE(qoderCn.hooksIsEntireFile(), false);
}
```

- [ ] **步骤 3：提交**

```bash
git add src/AiToolStrategy.h tests/test_ai_tool_strategy.cpp
git commit -m "feat: add hooksConfigPath() and hooksIsEntireFile() to AiToolStrategy"
```

---

### 任务 2：向 SettingsDialog 添加"编辑 Hooks 配置"按钮和编辑器对话框

**文件：**
- 修改：`src/SettingsDialog.h`
- 修改：`src/SettingsDialog.cpp`

- [ ] **步骤 1：在头文件中添加成员和槽函数声明**

在 `src/SettingsDialog.h` 中，在 `onShowHooksTemplate()` 之后添加一个新的私有槽函数：

```cpp
void onEditHooksConfig();
```

在 `QPushButton *m_hooksBtn;` 之后添加一个新成员：

```cpp
QPushButton *m_editHooksBtn;
```

- [ ] **步骤 2：创建按钮、添加到布局并连接信号**

在 `src/SettingsDialog.cpp` 构造函数中，将按钮部分（约第 111-118 行）从：

```cpp
// Buttons
m_hooksBtn = new QPushButton();
m_okBtn = new QPushButton();
m_cancelBtn = new QPushButton();
auto *btnLayout = new QHBoxLayout();
btnLayout->addWidget(m_hooksBtn);
btnLayout->addStretch();
btnLayout->addWidget(m_okBtn);
btnLayout->addWidget(m_cancelBtn);
```

改为：

```cpp
// Buttons
m_hooksBtn = new QPushButton();
m_editHooksBtn = new QPushButton();
m_okBtn = new QPushButton();
m_cancelBtn = new QPushButton();
auto *btnLayout = new QHBoxLayout();
btnLayout->addWidget(m_hooksBtn);
btnLayout->addWidget(m_editHooksBtn);
btnLayout->addStretch();
btnLayout->addWidget(m_okBtn);
btnLayout->addWidget(m_cancelBtn);
```

在现有的 `m_hooksBtn` 连接之后（约第 158-159 行）添加信号连接：

```cpp
connect(m_editHooksBtn, &QPushButton::clicked,
        this, &SettingsDialog::onEditHooksConfig);
```

- [ ] **步骤 3：为新按钮添加可翻译文本**

在 `retranslateUi()` 中，在 `m_hooksBtn->setText(...)`（第 213 行）之后添加：

```cpp
m_editHooksBtn->setText(tr("Edit Hooks Config"));
```

- [ ] **步骤 4：实现 onEditHooksConfig() 槽函数**

如果尚未存在，在 `src/SettingsDialog.cpp` 顶部添加这些 include：

```cpp
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFileInfo>
#include <QFont>
```

在文件末尾、`reject()` 之前添加该槽函数的实现：

```cpp
void SettingsDialog::onEditHooksConfig()
{
    const QString toolId = m_aiToolCombo->currentData().toString();
    auto *strategy = AiToolRegistry::find(toolId);
    if (!strategy)
        return;

    const QString configPath = strategy->hooksConfigPath();
    const bool entireFile = strategy->hooksIsEntireFile();

    // Read current content
    QString content;
    QFile file(configPath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray raw = file.readAll();
        file.close();
        if (entireFile) {
            content = QString::fromUtf8(raw);
        } else {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject hooksObj;
                hooksObj["hooks"] = doc.object()["hooks"];
                content = QJsonDocument(hooksObj).toJson(QJsonDocument::Indented);
            } else {
                content = strategy->hooksTemplate();
            }
        }
    } else {
        content = strategy->hooksTemplate();
    }

    // Build editor dialog
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Edit Hooks Config - %1").arg(strategy->displayName()));
    dlg->setMinimumSize(500, 400);

    auto *textEdit = new QTextEdit();
    textEdit->setPlainText(content);
    QFont monoFont("monospace");
    monoFont.setStyleHint(QFont::Monospace);
    textEdit->setFont(monoFont);

    auto *pathLabel = new QLabel(configPath);
    pathLabel->setWordWrap(true);

    auto *saveBtn = new QPushButton(tr("Save"));
    auto *cancelBtn = new QPushButton(tr("Cancel"));

    connect(saveBtn, &QPushButton::clicked, dlg, [this, dlg, textEdit, configPath, entireFile]() {
        const QString text = textEdit->toPlainText().trimmed();
        QJsonParseError err;
        QJsonDocument editedDoc = QJsonDocument::fromJson(text.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError) {
            QMessageBox::warning(dlg, tr("JSON Error"),
                tr("Invalid JSON at offset %1:\n%2").arg(err.offset).arg(err.errorString()));
            return;
        }

        // Prepare the final content to write
        QByteArray output;
        if (entireFile) {
            output = editedDoc.toJson(QJsonDocument::Indented);
        } else {
            // Read existing file to preserve non-hooks fields
            QJsonObject root;
            QFile existing(configPath);
            if (existing.exists() && existing.open(QIODevice::ReadOnly)) {
                QJsonDocument existingDoc = QJsonDocument::fromJson(existing.readAll());
                existing.close();
                if (existingDoc.isObject())
                    root = existingDoc.object();
            }
            // Merge hooks from edited content
            QJsonObject editedObj = editedDoc.object();
            if (editedObj.contains("hooks"))
                root["hooks"] = editedObj["hooks"];
            else
                root["hooks"] = editedObj;
            output = QJsonDocument(root).toJson(QJsonDocument::Indented);
        }

        // Create parent directory if needed
        QDir dir = QFileInfo(configPath).absoluteDir();
        if (!dir.exists())
            dir.mkpath(".");

        QFile outFile(configPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(output);
            outFile.close();
            dlg->accept();
        } else {
            QMessageBox::warning(dlg, tr("Save Error"),
                tr("Cannot write to: %1").arg(configPath));
        }
    });
    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(dlg);
    layout->addWidget(pathLabel);
    layout->addWidget(textEdit);
    layout->addLayout(btnLayout);

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}
```

- [ ] **步骤 5：提交**

```bash
git add src/SettingsDialog.h src/SettingsDialog.cpp
git commit -m "feat: add Edit Hooks Config button and editor dialog"
```

---

### 任务 3：更新翻译

**文件：**
- 修改：`translations/trafficlight4ai_zh.ts`
- 修改：`translations/trafficlight4ai_ja.ts`

- [ ] **步骤 1：添加中文翻译**

在 `translations/trafficlight4ai_zh.ts` 中，找到 `SettingsDialog` 上下文，并为以下内容添加条目：

- `"Edit Hooks Config"` → `"编辑 Hooks 配置"`
- `"Edit Hooks Config - %1"` → `"编辑 Hooks 配置 - %1"`
- `"JSON Error"` → `"JSON 错误"`
- `"Invalid JSON at offset %1:\n%2"` → `"偏移 %1 处 JSON 无效：\n%2"`
- `"Save Error"` → `"保存错误"`
- `"Cannot write to: %1"` → `"无法写入：%1"`

- [ ] **步骤 2：添加日文翻译**

在 `translations/trafficlight4ai_ja.ts` 中，找到 `SettingsDialog` 上下文，并为以下内容添加条目：

- `"Edit Hooks Config"` → `"Hooks 設定を編集"`
- `"Edit Hooks Config - %1"` → `"Hooks 設定を編集 - %1"`
- `"JSON Error"` → `"JSON エラー"`
- `"Invalid JSON at offset %1:\n%2"` → `"オフセット %1 で無効な JSON：\n%2"`
- `"Save Error"` → `"保存エラー"`
- `"Cannot write to: %1"` → `"書き込みできません：%1"`

- [ ] **步骤 3：提交**

```bash
git add translations/trafficlight4ai_zh.ts translations/trafficlight4ai_ja.ts
git commit -m "i18n: add translations for Edit Hooks Config feature"
```
