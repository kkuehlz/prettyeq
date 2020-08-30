#include "runguard.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <error.h>
#include <QtGlobal>
#include <QDebug>

RunGuard::RunGuard()
{
    ::memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    ::strncpy(&addr.sun_path[1], PRETTY_ABSTRACT_SOCK, sizeof(PRETTY_ABSTRACT_SOCK) - 2);
#pragma GCC diagnostic pop

    sockfd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0)
        qFatal("socket() failed: %s", strerror(sockfd));
}

bool RunGuard::isRunning()
{
    int r = ::bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
    if (r < 0) {
        if (errno == EADDRINUSE)
            return true;
        else
            qFatal("bind() failed: %s", strerror(errno));
    }
    return false;
}
