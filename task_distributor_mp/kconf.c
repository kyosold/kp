#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "kconf.h"
#include "klog.h"

config cfg; // 配置文件

int read_config(char *config_file, config *cfg)
{
    int ret = 0;
    dictionary *dict = open_conf_file(config_file);
    if (dict == NULL) {
        klog_error("open config file fail:%s", config_file);
        return 1;
    }

    // [master] ------
    struct conf_int_config master_int_all_array[] = {
        {"max_child_num", &cfg->master.max_child_num},
        {"exit_hours", &cfg->master.exit_hours},
        {"wait_task_seconds", &cfg->master.wait_task_seconds},
        {0, 0}
    };

    struct conf_str_config master_str_all_array[] = {
        {"log_level", cfg->master.log_level},
        {"task_file", cfg->master.task_file},
        {"task_offset_dir", cfg->master.task_offset_dir},
        {"whence_offset", cfg->master.whence_offset},
        {0, 0}
    };

    ret = parse_conf_file(dict,
                          "master",
                          master_int_all_array,
                          master_str_all_array);
    if (ret != 0) {
        klog_error("parse config file '%s' fail at '[master]'");
        close_conf_file(dict);
        return 1;
    }

    // [child] ------
    struct conf_int_config child_int_all_array[] = {
        {0, 0}
    };
    struct conf_str_config child_str_all_array[] = {
        {"log_level", cfg->child.log_level},
        {"bin_program", cfg->child.bin_program},
        {"task_proc_fail_dir", cfg->child.task_proc_fail_dir},
        {0, 0}
    };

    ret = parse_conf_file(dict,
                          "child",
                          child_int_all_array,
                          child_str_all_array);
    if (ret != 0) {
        klog_error("parse config file '%s' fail at '[child]'");
        close_conf_file(dict);
        return 1;
    }

    close_conf_file(dict);
    return 0;
}
