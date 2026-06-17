#include "Tl4aiClient.h"

#include <QLocalSocket>
#include <QtGlobal>

#include <cstdlib>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Tl4aiClient {

QByteArray defaultSocketPath()
{
#ifdef _WIN32
    return QByteArrayLiteral("trafficlight4ai");
#else
    // Priority: $XDG_RUNTIME_DIR, then $TMPDIR (macOS only), then /tmp
    const char *xdg = std::getenv("XDG_RUNTIME_DIR");
    if (xdg && xdg[0] != '\0')
        return QByteArray(xdg) + "/trafficlight4ai.sock";
#ifdef __APPLE__
    const char *tmpDir = std::getenv("TMPDIR");
    if (tmpDir && tmpDir[0] != '\0') {
        QByteArray dir(tmpDir);
        if (!dir.endsWith('/'))
            dir += '/';
        return dir + "trafficlight4ai.sock";
    }
#endif
    return QByteArray("/tmp/trafficlight4ai-") + QByteArray::number(getuid()) + ".sock";
#endif
}

void drainStdin()
{
#ifndef _WIN32
    // Drain stdin non-blocking to avoid broken pipe from callers (e.g. Codex hooks)
    int stdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (stdinFlags < 0)
        return;
    fcntl(STDIN_FILENO, F_SETFL, stdinFlags | O_NONBLOCK);
    char buf[4096];
    while (read(STDIN_FILENO, buf, sizeof(buf)) > 0) {}
    fcntl(STDIN_FILENO, F_SETFL, stdinFlags);
#endif
}

bool isStateCommand(const QString &command)
{
    const QString normalized = command.toLower();
    return normalized == QLatin1String("red")
        || normalized == QLatin1String("yellow")
        || normalized == QLatin1String("green");
}

int sendState(const QString &command)
{
    // Determine socket name/path: env var > default.
    QByteArray socketPath = qgetenv("TL4AI_SOCKET");
    if (socketPath.isEmpty())
        socketPath = defaultSocketPath();

    QByteArray payload = command.toUpper().toUtf8();
    payload += '\n';

    QLocalSocket socket;
    socket.connectToServer(QString::fromLocal8Bit(socketPath));
    if (!socket.waitForConnected(100))
        return 0;

    socket.write(payload);
    socket.waitForBytesWritten(100);
    socket.disconnectFromServer();
    return 0;
}

} // namespace Tl4aiClient
