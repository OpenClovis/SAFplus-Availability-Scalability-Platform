#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int getpeereid(int s, uid_t *euid, gid_t *gid)
{
    *euid = geteuid();
    *gid  = getgid();
    return (0);
}
