#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include "klog.h"
#include "kutils.h"
#include "kconf.h"
#include "task_process.h"
#include "my_process.h"

#define MAX_BUF 4096
char task_buf[MAX_BUF];

char sid[128];
char cfg_file[1024];
config cfg;

int ndelay_on(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}
int ndelay_off(int fd) {
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
}

void write_fail_queue(char *buf, char *fail_dir)
{
    int ret = kwrite_fail_queue(buf, fail_dir, sid);
    if (ret == 0)
        klog_error("save buf to fail queue succ.");
    else
        klog_error("save buf(%s) to fail queue error.", buf);
}

int main(int argc, char **argv)
{
    char buf[16] = {0};
    char *pbuf = buf;
    int r;
    int i = 0;

    klog_open(basename(argv[0]), LOG_MAIL, KLOG_DEBUG, NULL);

    if (argc != 3) {
        klog_error("arguments number error");
        return 1;
    }
    snprintf(sid, sizeof(sid), "%s", argv[1]);
    snprintf(cfg_file, sizeof(cfg_file), "%s", argv[2]);

    klog_set_sid(sid);

    // 读取配置文件
    int ret = read_config(cfg_file, &cfg);
    if (ret == 1) {
        klog_error("read config file:%s fail", cfg_file);
        return 1;
    }

    klog_debug("------ read config ------");
    klog_debug("[child]log_level: \"%s\"", cfg.child.log_level);
    klog_debug("[child]bin_program: \"%s\"", cfg.child.bin_program);
    klog_debug("[child]task_proc_fail_dir: \"%s\"", cfg.child.task_proc_fail_dir);
    klog_debug("-------------------------");

    if (*cfg.child.task_proc_fail_dir == '\0') {
        klog_error("set config [child]task_proc_fail_dir is empty");
        return 1;
    }

    if (strcasecmp(cfg.child.log_level, "debug") == 0)
        klog_set_level(KLOG_DEBUG);
    else if (strcasecmp(cfg.child.log_level, "info") == 0)
        klog_set_level(KLOG_INFO);


    // set stdin to ndelay
    ndelay_on(0);

    // read task buffer from master
    i = 0;
    memset(task_buf, 0, sizeof(task_buf));
    for (;;) {
        buf[0] = '\0';
        r = read(0, buf, sizeof(buf) - 1);
        klog_debug("read buf[%d]:%s", r, buf);
        if (r > 0) {
            strncat(task_buf, pbuf, r);
        } else {
            if (r == 0) {
                klog_error("master is closed, so i exit.");
                write_fail_queue(task_buf, cfg.child.task_proc_fail_dir);
                return 1;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                klog_debug("errno:%d", errno);
                // check is eof
                char *peof = &task_buf[strlen(task_buf) - 5];
                if (strcmp(peof, "\r\n.\r\n") == 0) {
                    *peof = '\0';
                    break;
                }
                continue;
            }
        }
    }

    klog_info("read task buf:[%d]%s", strlen(task_buf), task_buf);

    // process task
    ret = my_process(task_buf, strlen(task_buf));
    if (ret != 0) {
        klog_error("process task fail");
        write_fail_queue(task_buf, cfg.child.task_proc_fail_dir);
        return 1;
    }
    klog_info("process succ.");

    return 0;
}