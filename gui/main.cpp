#include "gui.h"
#include "runguard.h"
#include "unixsignalhandler.h"
#include <QApplication>
#include <QList>
#include <QObject>
#include <error.h>
#include <signal.h>

static void setupUnixSignalHandlers(QList<int> exitSignals) {
    struct sigaction sa;
    sigset_t blocking_mask;
    int r;

    sigemptyset(&blocking_mask);
    for (int sig : exitSignals) {
        r = sigaddset(&blocking_mask, sig);
        if (r < 0)
            qFatal("could not sigaddset(): %s", strerror(r));
    }

    sa.sa_handler = UnixSignalHandler::getInstance().exitHandler;
    sa.sa_mask = blocking_mask;
    sa.sa_flags = 0;
    sa.sa_flags |= SA_RESTART;

    for (int sig : exitSignals) {
        r = sigaction(sig, &sa, nullptr);
        if (r < 0)
            qFatal("could not sigaction(): %s", strerror(r));
    }
}

int main(int argc, char *argv[])
{
    RunGuard guard;
    if (guard.isRunning()) {
        qWarning() << "prettyeq is already running. Quitting!";
        return 0;
    }
    QApplication a(argc, argv);
    setupUnixSignalHandlers(QList<int>{SIGINT, SIGTERM, SIGQUIT, SIGABRT, SIGHUP});
    Gui w;
    QObject::connect(&a, SIGNAL(aboutToQuit()), &w, SLOT(cleanup()));
    w.show();
    return a.exec();
}
