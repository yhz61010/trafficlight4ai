#pragma once

#include <QByteArray>
#include <QString>

// Shared IPC client logic used by both the tl4ai-ctl CLI and the GUI binary's
// lightweight CLI-forwarding mode (e.g. AppImage hooks invoking `foo.AppImage red`).
// Keeping it in tl4ai_core avoids duplicating the socket-path resolution and the
// send routine across executables.
namespace Tl4aiClient {

// Compute the default IPC socket path/name for the current platform.
// Priority: $XDG_RUNTIME_DIR, then $TMPDIR (macOS only), then /tmp.
QByteArray defaultSocketPath();

// Non-blocking drain of stdin to avoid broken-pipe errors from callers that
// pipe JSON into the hook process (e.g. AI tool hooks). No-op on Windows.
void drainStdin();

// True if `command` is one of the recognized state words (case-insensitive):
// "red" / "yellow" / "green".
bool isStateCommand(const QString &command);

// Connect to the running GUI's IPC server and send a single state command
// (case-insensitive). The socket is resolved from $TL4AI_SOCKET when set,
// otherwise defaultSocketPath(). Returns 0 always, including when no server is
// reachable, so hook callers are never blocked. A QCoreApplication instance
// must already exist before calling this.
int sendState(const QString &command);

} // namespace Tl4aiClient
