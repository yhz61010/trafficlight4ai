#include <QCoreApplication>
#include <QString>

#include "Tl4aiClient.h"

int main(int argc, char *argv[])
{
    Tl4aiClient::drainStdin();

    if (argc < 2)
        return 0;

    QCoreApplication app(argc, argv);

    // Forward the raw argument; the server silently ignores unrecognized commands.
    return Tl4aiClient::sendState(QString::fromLocal8Bit(argv[1]));
}
