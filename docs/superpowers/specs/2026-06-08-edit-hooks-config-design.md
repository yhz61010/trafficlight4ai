# 编辑 Hooks 配置设计文档

## 目标

在设置对话框中，于现有的 "View Recommended Hooks Config" 按钮旁边新增一个 "Edit Hooks Config" 按钮。该按钮会为所选 AI 工具打开其 hooks 配置文件的编辑器，并采用智能的读写逻辑以处理不同的文件结构。

## AiToolStrategy 接口变更

向 `AiToolStrategy` 新增两个虚方法：

- `hooksConfigPath()` — 返回该工具 hooks 配置文件的绝对路径。
- `hooksIsEntireFile()` — 当整个文件即为 hooks JSON（Codex）时返回 `true`；当 hooks 是某个嵌套字段（Claude Code、Qoder CN）时返回 `false`。

| 工具       | `hooksConfigPath()`          | `hooksIsEntireFile()` |
|------------|------------------------------|-----------------------|
| Codex      | `~/.codex/hooks.json`        | `true`                |
| Claude Code| `~/.claude/settings.json`    | `false`               |
| Qoder CN   | `~/.qoder-cn/settings.json`  | `false`               |

## 编辑器对话框行为

### 读取逻辑

1. 若文件存在且 `hooksIsEntireFile() == true`：将整个文件内容加载到编辑器中。
2. 若文件存在且 `hooksIsEntireFile() == false`：解析 JSON，提取 `"hooks"` 字段，格式化为带缩进的 JSON 并以 `{"hooks": ...}` 包裹，在编辑器中显示。
3. 若文件不存在：用 `hooksTemplate()` 的输出预填充编辑器。

### 保存逻辑

1. 保存前校验 JSON。若无效，显示错误提示且不保存。
2. 若 `hooksIsEntireFile() == true`：将编辑器内容直接写入文件。
3. 若 `hooksIsEntireFile() == false`：读取原始文件（或空的 `{}`），解析编辑器内容以提取 hooks 对象，将其合并到原始 JSON 的 `"hooks"` 键下，再写回。
4. 若父目录不存在则创建之。

### UI

- 新增一个 `QPushButton` "Edit Hooks Config"，在水平布局中放置于现有的 "View Recommended Hooks Config" 按钮旁边。
- 编辑器对话框：`QDialog`，内含 `QTextEdit`（可读写）、"Save" 和 "Cancel" 按钮。
- 对话框标题："Edit Hooks Config - {tool display name}"。
- `QTextEdit` 使用等宽字体以便编辑 JSON。
- 两个按钮均通过 `tr()` 支持翻译。

## 待修改文件

- `src/AiToolStrategy.h` — 向接口及全部三个策略类添加 `hooksConfigPath()` 和 `hooksIsEntireFile()`。
- `src/SettingsDialog.h` — 添加 `m_editHooksBtn` 成员和 `onEditHooksConfig()` 槽函数。
- `src/SettingsDialog.cpp` — 添加按钮创建、布局、连接以及编辑器对话框的实现。
- `translations/trafficlight4ai_zh.ts` — 为新增字符串添加中文翻译。
- `translations/trafficlight4ai_ja.ts` — 为新增字符串添加日文翻译。
- `tests/test_ai_tool_strategy.cpp` — 为 `hooksConfigPath()` 和 `hooksIsEntireFile()` 添加测试。

## 测试

- 验证 `hooksConfigPath()` 为全部三个工具返回预期的路径。
- 验证 `hooksIsEntireFile()` 返回正确的值。
- 手动测试：为每个工具打开编辑器，验证文件缺失时的预填充、保存，并重新打开以确认持久化生效。
