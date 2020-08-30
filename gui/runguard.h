#ifndef RUNGUARD_H
#define RUNGUARD_H

#include <sys/un.h>
#define PRETTY_ABSTRACT_SOCK "prettyeq"

/* Linux only implementation of a single application guard.
 * We bind to an abstract socket so cleanup is handled entirely
 * by the kernel. */
class RunGuard
{
public:
    RunGuard();
    bool isRunning();

private:
    int sockfd;
    struct sockaddr_un addr;
};

#endif // RUNGUARD_H
