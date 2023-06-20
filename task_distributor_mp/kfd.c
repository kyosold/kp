#include <fcntl.h>
#include <unistd.h>
#include "kfd.h"

int kfd_copy(int from, int to)
{
    if (from == to)
        return 0;
    if (fcntl(from, F_GETFL, 0) == -1)
        return -1;
    close(to);
    if (fcntl(from, F_DUPFD, to) == -1)
        return -1;
    return 0;
}

int kfd_move(int from, int to)
{
    if (from == to)
        return 0;
    if (kfd_copy(from, to) == -1)
        return -1;
    close(from);
    return 0;
}