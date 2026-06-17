# 配置 AI 工具的 Hooks

以下示例使用短名 `tl4ai-ctl`。如果你将 trafficlight4ai 安装到了自定义位置，请替换为完整路径。AppImage 场景请使用设置对话框显示的真实 `.AppImage` 路径（例如 `/path/to/trafficlight4ai-<version>-linux-amd64.AppImage red`），不要保存临时的 `/tmp/.mount_*` 路径——它每次启动都会变化。

也可以在设置对话框中点击"查看推荐 Hooks 配置"按钮获取模板，或点击"编辑 Hooks 配置"按钮直接编辑。

## Codex

创建 `~/.codex/hooks.json`：

```json
{
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
}
```

## Claude Code

添加到 `~/.claude/settings.json`：

```json
{
  "hooks": {
    "UserPromptSubmit": [{ "command": "tl4ai-ctl red" }],
    "PreToolUse": [{ "command": "tl4ai-ctl red" }],
    "SubagentStart": [{ "command": "tl4ai-ctl red" }],
    "Notification": [{ "command": "tl4ai-ctl yellow" }],
    "PermissionRequest": [{ "command": "tl4ai-ctl yellow" }],
    "Stop": [{ "command": "tl4ai-ctl green" }],
    "SessionEnd": [{ "command": "tl4ai-ctl green" }]
  }
}
```

## Qoder CN

添加到 `~/.qoder-cn/settings.json`（或项目级 `.qoder-cn/settings.json`）：

```json
{
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
}
```

## Copilot

保存为 `~/.copilot/hooks/trafficlight4ai.json`：

```json
{
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
}
```

## Gemini

添加到 `~/.gemini/settings.json`：

```json
{
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
}
```
