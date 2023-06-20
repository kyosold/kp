#ifndef __KCONF_H__
#define __KCONF_H__

#include "confparser.h"
#include "dictionary.h"

#define MAX_LINE 1024

typedef struct config_st
{
    struct {
        unsigned int max_child_num;
        char log_level[MAX_LINE];
        char task_file[MAX_LINE];
        char task_offset_dir[MAX_LINE];
        char whence_offset[MAX_LINE];
        int exit_hours;
        int wait_task_seconds;
    } master;

    struct {
        char log_level[MAX_LINE];
        char bin_program[MAX_LINE];
        char task_proc_fail_dir[MAX_LINE];
    } child;

} config;

int read_config(char *config_file, config *cfg);

#endif