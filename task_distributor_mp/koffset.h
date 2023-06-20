#ifndef __KOFFSET_H__
#define __KOFFSET_H__

#include <sys/types.h>

#define OFFSET_SIZE 1024

typedef struct task_offset_st
{
    int fd;       // 被读取文件当前偏移量的文件描述符
    char *ptr;    // 偏移量，用于共享内存到文件
    off_t offset; // 偏移量
} task_offset;

int init_task_offset(task_offset *t, char *task_offset_path);

#endif