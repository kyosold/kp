#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <libgen.h>
#include "kutils.h"
#include "klog.h"
#include "kconf.h"
#include "koffset.h"
#include "kfd.h"

#define MAX_LINE    1024
#define BUFSIZE 4096

config cfg;
task_offset offset;
int active_processes = 0;   // 当前活动的子进程数量
char cfg_file[MAX_LINE] = {0};  // config file
char sid[128];

void process_task(FILE *fp, char *buf)
{
    // 检查是否达到最大子进程数量限制
    klog_debug("child numbers: %d/%d", active_processes, cfg.master.max_child_num);
    while (active_processes >= cfg.master.max_child_num) {
        klog_info("reached maximum number of child process: %d/%d. Waiting for some to finish", active_processes, cfg.master.max_child_num);
        wait(NULL); // 等待任一子进程结束
        active_processes--;
    }

    // 设置父p[1]->子通道p[0]
    int pifd[2];
    if (pipe(pifd) == -1) {
        klog_error("pipe fail:%m");
        int ret = kwrite_fail_queue(buf, cfg.child.task_proc_fail_dir, sid);
        if (ret == 0)
            klog_error("save buf to fail queue succ.");
        else
            klog_error("save buf(%s) to fail queue error.", buf);
        return;
    }
    fcntl(pifd[1], F_SETFD, 1);  // set close exec

    char cid[128];
    kgen_uuid(cid, sizeof(cid), 1);

    // child args
    char *(args[4]);
    args[0] = cfg.child.bin_program;    // task_process
    args[1] = cid;
    args[2] = cfg_file; // config file name
    args[3] = 0;

    pid_t pid;
    while ((pid = fork()) == -1) {
        klog_error("fork fail:%m");
        sleep(cfg.master.wait_task_seconds);
    }
    if (pid == 0) {
        // 子进程
        fclose(fp);
        close(pifd[1]);

        if (kfd_move(pifd[0], 0) == -1) {
            klog_error("kfd_move(pifd, 0) fail:%m");
            _exit(120);
        }

        int ret = execvp(*args, args);
        if (ret == -1) {
            klog_error("execvp: %s fail:%m", *args);
            _exit(errno);
        }

        _exit(0);
    } else {
        // 父进程
        close(pifd[0]);
        active_processes++;

        int w = write(pifd[1], buf, strlen(buf));           // write: content
        write(pifd[1], "\r\n.\r\n", 5);             // use `\r\n.\r\n` is end

        klog_info("write to child ci:%s buf:[%d]%s", cid, w, buf);

        klog_info("start child: %d/%d", active_processes, cfg.master.max_child_num);
    }
}

int is_master_exit()
{
    struct stat st;
    if (stat(cfg.master.task_file, &st) == -1) {
        klog_error("stat task file:%s fail:%m", cfg.master.task_file);
        return 0;
    }

    // 几个小时内，任务没有更新就退出
    time_t now = time(NULL);
    time_t mtime = st.st_mtime + (3600 * cfg.master.exit_hours);
    if (now > mtime)
    {
        klog_info("task file:%s was expire(%lld/%lld). bye.", cfg.master.task_file, now, mtime);
        return 1;
    }
    return 0;
}

void usage(char *prog)
{
    printf("Usage:\n");
    printf("  %s -c <config file>\n", prog);
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    int ch;

    kgen_uuid(sid, sizeof(sid), 1);

    klog_open(basename(argv[0]), LOG_MAIL, KLOG_DEBUG, sid);

    // process command line args
    const char *args = "c:h";
    while ((ch = getopt(argc, argv, args)) != -1) {
        switch (ch) {
        case 'c':
            snprintf(cfg_file, sizeof(cfg_file), "%s", optarg);
            break;
        case 'h':
        default:
            usage(argv[0]);
            exit(1);
        };
    }

    struct stat cfg_st;
    if (stat(cfg_file, &cfg_st) == -1) {
        klog_error("config file:%s fail:%m", cfg_file);
        return 1;
    }
    if ((cfg_st.st_mode & S_IFMT) != S_IFREG) {
        klog_error("config file:%s is not normal file", cfg_file);
        return 1;
    }

    // 读取配置文件
    int ret = read_config(cfg_file, &cfg);
    if (ret == 1) {
        klog_error("read config file:%s fail", cfg_file);
        return 1;
    }

    klog_info("------ read config ------");
    klog_info("[master]max_child_num: %d", cfg.master.max_child_num);
    klog_info("[master]log_level: \"%s\"", cfg.master.log_level);
    klog_info("[master]task_file: \"%s\"", cfg.master.task_file);
    klog_info("[master]task_offset_dir: \"%s\"", cfg.master.task_offset_dir);
    klog_info("[master]whence_offset: \"%s\"", cfg.master.whence_offset);
    klog_info("[master]exit_hours: %d", cfg.master.exit_hours);
    klog_info("[master]wait_task_seconds: %d", cfg.master.wait_task_seconds);
    klog_info("[child]bin_program: \"%s\"", cfg.child.bin_program);
    klog_info("[child]task_proc_fail_dir: \"%s\"", cfg.child.task_proc_fail_dir);
    klog_info("-------------------------");

    if (*cfg.master.task_file == '\0') {
        klog_error("set config [master]task_file is empty");
        return 1;
    }

    if (strcasecmp(cfg.master.log_level, "debug") == 0)
        klog_set_level(KLOG_DEBUG);
    else if (strcasecmp(cfg.master.log_level, "info") == 0)
        klog_set_level(KLOG_INFO);

    if (cfg.master.max_child_num <= 0) {
        printf("set 'max_child_num' error");
        return 1;
    }

    // 初始化偏移文件
    if (init_task_offset(&offset, cfg.master.task_offset_dir) != 0) {
        printf("init task offset fail\n");
        return 1;
    }

    // 打开任务文件
    char buf[BUFSIZE] = {0};
    FILE *fp = fopen(cfg.master.task_file, "r");
    if (!fp) {
        printf("open file:%s fail:%m\n", cfg.master.task_file);
        if (offset.fd)
            close(offset.fd);
        return 1;
    }

    // 如果偏移量大于0，则偏移文件到上次读取的地方
    if (strcasecmp(cfg.master.whence_offset, "continue") == 0) {
        if (offset.offset > 0) {
            if (fseek(fp, offset.offset, SEEK_SET) == -1) {
                printf("seek to offset %lld fail:%m\n", offset.offset);
                fclose(fp);
                if (offset.fd)
                    close(offset.fd);
                return 1;
            }
        }
    } else if (strcasecmp(cfg.master.whence_offset, "end") == 0) {
        // 偏移到文件尾部
        fseek(fp, 0, SEEK_END);
    } else if (strcasecmp(cfg.master.whence_offset, "begin") == 0) {
        // 偏移到文件头文件
        fseek(fp, 0, SEEK_SET);
    } else {
        // 不明白
        printf("not support args: [mast]whence_offset: %s\n", cfg.master.whence_offset);

        fclose(fp);
        close(offset.fd);
        return 1;
    }
    printf("%s started.\n", argv[0]);

    // 循环读取任务，读取一行，创建一个进程去执行
    for (;;) {
        if (!fgets(buf, sizeof(buf) - 1, fp)) {
            // 读取不到
            if (is_master_exit()) {
                // 退出
                klog_info("I am exit. bye!");
                goto BYEBYE;
            }

            klog_info("fget is null, wait %ds for new task", cfg.master.wait_task_seconds);
            sleep(cfg.master.wait_task_seconds);
            continue;
        }
        if (*buf == '\0')
            continue;

        // 保存当前的偏移量
        offset.offset = ftell(fp);
        if (offset.offset > 0) {
            snprintf(offset.ptr, OFFSET_SIZE, "%ld", offset.offset);
            klog_debug("offset ptr:%s", offset.ptr);
        }

        // 处理任务
        process_task(fp, buf);
    }

BYEBYE:
    // 等待所有子进程结束
    while (active_processes > 0) {
        wait(NULL);
        active_processes--;
    }

    fclose(fp);
    if (offset.fd)
        close(offset.fd);

    printf("%s end.\n", argv[0]);
    return 0;
}


