#ifndef UNIXSIGNALHANDLER_H
#define UNIXSIGNALHANDLER_H

#include <QObject>
#include <QSocketNotifier>

class UnixSignalHandler : public QObject
{
    Q_OBJECT
public:
    static UnixSignalHandler& getInstance();
    ~UnixSignalHandler();
    UnixSignalHandler(UnixSignalHandler const&) = delete;
    void operator=(UnixSignalHandler const&)    = delete;

private:
    explicit UnixSignalHandler(QObject *parent = nullptr);

public:
    /* SIGTERM, SIGINT, SIGQUIT, SIGABRT */
    static void exitHandler(int sig);

public slots:
    void safeHandleExit();

private:
    static UnixSignalHandler instance;
    /* All signals share the same socketpair and will mask each other. */
    static int socketVector[2];
    QSocketNotifier *sn;
};

#endif // UNIXSIGNALHANDLER_H
