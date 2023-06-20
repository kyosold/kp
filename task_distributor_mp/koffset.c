#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "koffset.h"
#include "klog.h"
#include "kutils.h"

#define OFFSET_FILE "read.offset"

int init_task_offset(task_offset *t, char *task_offset_path)
{
    char path[1024] = {0};
    char offset_file[1024] = {0};

    t->fd = 0;
    t->ptr = NULL;
    t->offset = 0;

    struct tm *now;
    struct stat st;

    char now_str[1024] = {0};
    time_t cur = time(NULL);

    now = localtime(&cur);
    strftime(now_str, sizeof(now_str) - 1, "%Y%m%d", now);

    if (task_offset_path == NULL) {
        path[0] = '.';
        path[1] = '\0';
    } else {
        snprintf(path, sizeof(path), task_offset_path);
        if (kmkdirs(path, 0644) != 0) {
            klog_error("create task offset path '%s' fail", path);
            return 1;
        }
    }

    snprintf(offset_file, sizeof(offset_file), "%s/%s.%s",
             path, OFFSET_FILE, now_str);

    t->fd = open(offset_file, O_RDWR | O_NDELAY | O_CREAT, 0600);
    if (t->fd == -1) {
        klog_error("open offset file:%s fail:%m", offset_file);
        return 1;
    }

    fcntl(t->fd, F_SETFD, 1);   // 禁止与子进程共享

    if (fstat(t->fd, &st) == -1) {
        klog_error("fstat file:%s fail:%m", offset_file);
        close(t->fd);
        return 1;
    }

    if (st.st_size < 1) {
        ftruncate(t->fd, 1024);
    }

    t->ptr = mmap(NULL,
                  OFFSET_SIZE,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED,
                  t->fd,
                  0);
    if (t->ptr == MAP_FAILED)
    {
        klog_error("mmap file:%s fail:%m", offset_file);
        close(t->fd);
        return 1;
    }

    if (st.st_size > 0) {
        t->offset = atoll(t->ptr);
    }

    return 0;
}