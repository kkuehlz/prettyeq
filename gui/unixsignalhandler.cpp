#include "unixsignalhandler.h"
#include "prettyshim.h"
#include <QDebug>
#include <QCoreApplication>

#include <sys/socket.h>
#include <unistd.h>

int UnixSignalHandler::socketVector[2];

UnixSignalHandler &UnixSignalHandler::getInstance() {
    static UnixSignalHandler instance;
    return instance;
}

UnixSignalHandler::UnixSignalHandler(QObject *parent) : QObject(parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, socketVector))
        qFatal("Could not socketpair();");

    sn = new QSocketNotifier(socketVector[1], QSocketNotifier::Read, this);
    QObject::connect(sn, SIGNAL(activated(QSocketDescriptor, QSocketNotifier::Type)), this, SLOT(safeHandleExit()));
}

void UnixSignalHandler::exitHandler(int sig)
{
    /* Write junk to the socket to trigger Qt signal */
    char junk = 1;
    ::write(socketVector[0], &junk, sizeof(char));
}

void UnixSignalHandler::safeHandleExit()
{
    sn->setEnabled(false);
    char junk;
    ::read(socketVector[1], &junk, sizeof(char));
    QCoreApplication::quit();
    sn->setEnabled(true);
}

UnixSignalHandler::~UnixSignalHandler()
{
    delete sn;
}
