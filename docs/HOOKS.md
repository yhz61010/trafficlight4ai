# Configure Hooks for Your AI Tool

The examples below use the short name `tl4ai-ctl`. If you installed trafficlight4ai to a custom location, replace it with the full path. For the AppImage, use the real `.AppImage` path shown by the Settings dialog (e.g. `/path/to/trafficlight4ai-<version>-linux-amd64.AppImage red`); do not save the temporary `/tmp/.mount_*` path, which changes on every launch.

You can also view these templates in the Settings dialog ("View Recommended Hooks Config" button) or edit them directly ("Edit Hooks Config" button).

## Codex

Create `~/.codex/hooks.json`:

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

Add to `~/.claude/settings.json`:

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

Add to `~/.qoder-cn/settings.json` (or project-level `.qoder-cn/settings.json`):

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

Save as `~/.copilot/hooks/trafficlight4ai.json`:

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

Add to `~/.gemini/settings.json`:

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
