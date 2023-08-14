#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "kutils.h"

#define VERSION "1.0.0"

#define COLOR_NONE "\033[0m"
#define COLOR_RED "\033[1;31;40m"
#define BUF_SIZE 4096
unsigned char eol_byte = '\n';

typedef struct matcher_st
{
    int day;
    int start_hour;
    int start_minute;
    int end_hour;
    int end_minute;
    unsigned long long min_size;
    int verbose;
    int debug;
    char filename[BUF_SIZE];
} matcher;

typedef struct found_pos_st
{
    unsigned long long pos;
    unsigned long long seconds;
    char time_str[128];
    char day_str[64];
} found_pos;

unsigned long long get_seconds_line(char *buf, char *line_time, size_t line_time_size)
{
    // Jun 15 10:44:28 smailadmin-23-184 smail_kf_sent
    unsigned long long res = 0;
    char day_str[64] = {0};
    char time_str[128] = {0};
    int space_i = 0;
    int i = 0;
    int j = 0;
    int d = 0;
    for (i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == ' ')
        {
            if (buf[i + 1] == ' ')
                continue;
            else
            {
                space_i++;
                continue;
            }
        }

        if (space_i == 1)
        {
            day_str[d++] = buf[i];
            day_str[d] = 0;
            continue;
        }
        if (space_i == 2)
        {
            time_str[j++] = buf[i];
            time_str[j] = 0;
            continue;
        }
        if (space_i > 2)
            break;
    }

    // printf("get time[%s] from buf(%s)\n", time, buf);
    space_i = 0;
    i = 0;
    j = 0;
    char str[10];
    for (i = 0; i < strlen(time_str); i++)
    {
        if (time_str[i] == ':')
        {
            str[j] = 0;
            if (space_i == 0)
            {
                res = atoi(str) * 3600;
                // printf("hour:%s -> %llu\n", str, res);
                j = 0;
            }
            else if (space_i == 1)
            {
                res += atoi(str) * 60;
                // printf("min:%s -> %llu\n", str, res);
                j = 0;
            }
            space_i++;
            continue;
        }
        str[j++] = time_str[i];
    }

    if (space_i == 2)
    {
        res += atoi(str);
        // printf("sec:%s -> %llu\n", str, res);
    }
    // strncpy(line_time, time, strlen(time));
    snprintf(line_time, line_time_size, "(%d) %s", atoi(day_str), time_str);

    res += (atoi(day_str) * 86400);

    return res;
}

int get_start_pos(matcher *ma, found_pos *fpos)
{
    char buf[BUF_SIZE];
    char line_time[128];
    int is_bigger = 0;
    int is_smaller = 0;
    found_pos pre_fpos;

    struct stat st;
    if (stat(ma->filename, &st) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    // init pos
    unsigned long long s_pos = 0;
    unsigned long long e_pos = st.st_size;
    unsigned long long c_pos = e_pos / 2;
    unsigned long long line_seconds = 0;

    // init fpos
    fpos->pos = 0;
    fpos->seconds = 0;
    // init pre fpos
    pre_fpos.pos = 0;
    pre_fpos.seconds = 0;

    // 是否直接从开头开始
    if (ma->start_hour == 0 && ma->start_minute == 0)
    {
        // 直接从文件头开始
        fpos->pos = 0;
        fpos->seconds = 0;
        if (ma->verbose > 0)
        {
            printf("[Start] Match: seconds(0),pos(0)\n");
        }
        return 0;
    }

    // 计算要匹配的时间秒数
    if (ma->start_hour > 0)
    {
        fpos->seconds = (ma->start_hour * 3600);
    }
    if (ma->start_minute > 0)
    {
        fpos->seconds += (ma->start_minute * 60);
    }

    int today = ma->day;
    fpos->seconds += (today * 86400);
    snprintf(fpos->time_str, sizeof(fpos->time_str) - 1, "(%d) %02d:%02d", today, ma->start_hour, ma->start_minute);
    if (ma->verbose > 0)
    {
        printf("[Start] Match: seconds(%llu[%d:%d])\n", fpos->seconds, ma->start_hour, ma->start_minute);
    }

    FILE *fp = fopen(ma->filename, "r");
    if (fp == NULL)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    // 先看第1行是否比要匹配的时间大，如果大直接返回第1行
    if (fgets(buf, sizeof(buf) - 1, fp) != NULL)
    {
        line_seconds = get_seconds_line(buf, line_time, sizeof(line_time));
        if (line_seconds >= fpos->seconds)
        {
            // 没有再靠前的了，所以第1行就是开始
            fpos->pos = 0;
            fpos->seconds = line_seconds;
            // strncpy(fpos->time_str, line_time, strlen(line_time));
            snprintf(fpos->time_str, sizeof(fpos->time_str), "%s", line_time);
            goto FINAL_SUCC;
        }
    }

    // 偏移到中间
    fseek(fp, c_pos, SEEK_SET);
    fgets(buf, sizeof(buf) - 1, fp); // 取下一行

    while (fgets(buf, sizeof(buf) - 1, fp) != NULL)
    {
        // 获取当前行时间
        line_seconds = get_seconds_line(buf, line_time, sizeof(line_time));

        if (line_seconds > fpos->seconds)
        {
            // 往前找，已经超过需要查找的点了
            if (ma->verbose > 0)
            {
                printf("[Start] Check B: seconds(%llu[%s]) >>> seconds(%llu[%s]), pos(%llu, %llu, %llu) psize(%llu)\n",
                       fpos->seconds, fpos->time_str, line_seconds, line_time, s_pos, c_pos, e_pos, (e_pos - s_pos));
            }

            is_bigger = 1;
            if (is_smaller == 1)
            {
                is_smaller = 0;

                e_pos = c_pos;
                s_pos = pre_fpos.pos;

                // 判断是否微调
                // printf("   A size(%d)\n", (e_pos - s_pos));
                if ((e_pos - s_pos) <= ma->min_size)
                {
                    // 不需要
                    fpos->pos = pre_fpos.pos;
                    fpos->seconds = pre_fpos.seconds;
                    // strncpy(fpos->time_str, pre_fpos.time_str, strlen(pre_fpos.time_str));
                    snprintf(fpos->time_str, sizeof(fpos->time_str) - 1, "%s", pre_fpos.time_str);
                    goto FINAL_SUCC;
                }
                // 开始微调
                c_pos = (e_pos - s_pos) / 2 + s_pos;
                fseek(fp, c_pos, SEEK_SET);
                fgets(buf, sizeof(buf) - 1, fp); // 取下一行

                // 保存当前点为前一个
                pre_fpos.pos = e_pos;
                pre_fpos.seconds = line_seconds;
                // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
                snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

                continue;
            }

            // 保存当前点为前一个
            pre_fpos.pos = c_pos;
            pre_fpos.seconds = line_seconds;
            // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
            snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

            // 偏移
            e_pos = c_pos;
            c_pos = (e_pos - s_pos) / 2 + s_pos;
            fseek(fp, c_pos, SEEK_SET);
            fgets(buf, sizeof(buf) - 1, fp); // 取下一行
            continue;
        }
        else
        {
            // 往后找，还没到需要查找的点
            if (ma->verbose > 0)
            {
                printf("[Start] Check A: seconds(%llu[%s]) >>> seconds(%llu[%s]), pos(%llu, %llu, %llu) psize(%llu)\n",
                       fpos->seconds, fpos->time_str, line_seconds, line_time, s_pos, c_pos, e_pos, (e_pos - s_pos));
            }

            is_smaller = 1;
            if (is_bigger == 1)
            {
                is_bigger = 0;

                // 偏移
                e_pos = pre_fpos.pos;
                s_pos = c_pos;

                // 判断是否微调
                // printf("   B size(%d)\n", (e_pos - s_pos));
                if ((e_pos - s_pos) <= ma->min_size)
                {
                    // 不需要
                    fpos->pos = c_pos;
                    fpos->seconds = line_seconds;
                    // strncpy(fpos->time_str, line_time, strlen(line_time));
                    snprintf(fpos->time_str, sizeof(fpos->time_str) - 1, "%s", line_time);
                    goto FINAL_SUCC;
                }
                // 开始微调
                c_pos = (e_pos - s_pos) / 2 + s_pos;
                fseek(fp, c_pos, SEEK_SET);
                fgets(buf, sizeof(buf) - 1, fp); // 取下一行

                // 保存当前点为前一个
                pre_fpos.pos = s_pos;
                pre_fpos.seconds = line_seconds;
                // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
                snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

                continue;
            }

            // 保存当前点为前一个
            pre_fpos.pos = c_pos;
            pre_fpos.seconds = line_seconds;
            // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
            snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

            // 偏移
            s_pos = c_pos;
            c_pos = (e_pos - s_pos) / 2 + s_pos;
            fseek(fp, c_pos, SEEK_SET);
            fgets(buf, sizeof(buf) - 1, fp); // 取下一行
            continue;
        }
    }

FINAL_FAIL:
    fclose(fp);
    return -1;

FINAL_SUCC:
    fclose(fp);
    return 0;
}

int get_end_pos(matcher *ma, unsigned long long start_pos, found_pos *fpos)
{
    char buf[BUF_SIZE];
    char line_time[128];
    int is_bigger = 0;
    int is_smaller = 0;
    found_pos pre_fpos;

    struct stat st;
    if (stat(ma->filename, &st) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    // init pos
    unsigned long long s_pos = start_pos;
    unsigned long long e_pos = st.st_size;
    unsigned long long c_pos = ((e_pos - s_pos) / 2) + s_pos;
    unsigned long long line_seconds = 0;

    // init fpos
    fpos->pos = 0;
    fpos->seconds = 0;
    // init pre fpos
    pre_fpos.pos = 0;
    pre_fpos.seconds = 0;

    // 是否到文件结尾
    if (ma->end_hour == -1 && ma->end_minute == -1)
    {
        // 直接到文件结尾
        fpos->pos = st.st_size;
        if (ma->verbose > 0)
        {
            printf("[End] Match: seconds(-1[eof]),pos(%llu)\n", st.st_size);
        }
        return 0;
    }

    // 计算要匹配的时间秒数
    if (ma->end_hour > 0)
    {
        fpos->seconds = (ma->end_hour * 3600);
    }
    if (ma->end_minute > 0)
    {
        fpos->seconds += (ma->end_minute * 60);
    }

    int today = ma->day;
    fpos->seconds += (today * 86400);
    snprintf(fpos->time_str, sizeof(fpos->time_str), "(%d) %02d:%02d", today, ma->end_hour, ma->end_minute);
    if (ma->verbose > 0)
    {
        printf("[End] Match: seconds(%llu[%d:%d])\n", fpos->seconds, ma->start_hour, ma->start_minute);
    }

    FILE *fp = fopen(ma->filename, "r");
    if (fp == NULL)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    // 偏移到中间
    fseek(fp, c_pos, SEEK_SET);
    fgets(buf, sizeof(buf) - 1, fp); // 取下一行

    while (fgets(buf, sizeof(buf) - 1, fp) != NULL)
    {
        // 获取当前行时间
        line_seconds = get_seconds_line(buf, line_time, sizeof(line_time));

        if (line_seconds > fpos->seconds)
        {
            // 往前找，已经超过需要查找的点了
            if (ma->verbose > 0)
            {
                printf("[End] Check B: seconds(%llu[%s]) >>> seconds(%llu[%s]), pos(%llu, %llu, %llu) psize(%llu)\n",
                       fpos->seconds, fpos->time_str, line_seconds, line_time, s_pos, c_pos, e_pos, (e_pos - s_pos));
            }

            is_bigger = 1;
            if (is_smaller)
            {
                is_smaller = 0;

                e_pos = c_pos;
                s_pos = pre_fpos.pos;

                // 判断是否微调
                // printf("   A size(%d)\n", (e_pos - s_pos));
                if ((e_pos - s_pos) <= ma->min_size)
                {
                    // 不需要
                    fpos->pos = c_pos;
                    fpos->seconds = line_seconds;
                    snprintf(fpos->time_str, sizeof(fpos->time_str) - 1, "%s", line_time);
                    goto FINAL_SUCC;
                }

                // 开始微调
                c_pos = (e_pos - s_pos) / 2 + s_pos;
                fseek(fp, c_pos, SEEK_SET);
                fgets(buf, sizeof(buf) - 1, fp); // 取下一行

                // 保存当前点为前一个
                pre_fpos.pos = e_pos;
                pre_fpos.seconds = line_seconds;
                // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
                snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

                continue;
            }

            // 保存当前点为前一个
            pre_fpos.pos = c_pos;
            pre_fpos.seconds = line_seconds;
            // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
            snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

            // 偏移
            e_pos = c_pos;
            c_pos = (e_pos - s_pos) / 2 + s_pos;
            fseek(fp, c_pos, SEEK_SET);
            fgets(buf, sizeof(buf) - 1, fp); // 取下一行
            continue;
        }
        else
        {
            // 往后找，还没到需要查找的点
            if (ma->verbose > 0)
            {
                printf("[End] Check A: seconds(%llu[%s]) >>> seconds(%llu[%s]), pos(%llu, %llu, %llu) psize(%llu)\n",
                       fpos->seconds, fpos->time_str, line_seconds, line_time, s_pos, c_pos, e_pos, (e_pos - s_pos));
            }

            if ((e_pos - s_pos) <= 1)
            {
                is_bigger = 1;
            }

            is_smaller = 1;
            if (is_bigger)
            {
                is_bigger = 0;

                // 偏移
                e_pos = pre_fpos.pos;
                s_pos = c_pos;

                // 判断是否微调
                // printf("   B size(%d)\n", (e_pos - s_pos));
                if ((e_pos - s_pos) <= ma->min_size)
                {
                    // 不需要
                    fpos->pos = pre_fpos.pos;
                    fpos->seconds = pre_fpos.seconds;
                    // strncpy(fpos->time_str, pre_fpos.time_str, strlen(pre_fpos.time_str));
                    snprintf(fpos->time_str, sizeof(fpos->time_str) - 1, "%s", pre_fpos.time_str);
                    goto FINAL_SUCC;
                }

                // 开始微调
                c_pos = (e_pos - s_pos) / 2 + s_pos;
                fseek(fp, c_pos, SEEK_SET);
                fgets(buf, sizeof(buf) - 1, fp); // 取下一行

                // 保存当前点为前一个
                pre_fpos.pos = s_pos;
                pre_fpos.seconds = line_seconds;
                // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
                snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str) - 1, "%s", line_time);

                continue;
            }

            // 保存当前点为前一个
            pre_fpos.pos = c_pos;
            pre_fpos.seconds = line_seconds;
            // strncpy(pre_fpos.time_str, line_time, strlen(line_time));
            snprintf(pre_fpos.time_str, sizeof(pre_fpos.time_str), "%s", line_time);

            // 偏移
            s_pos = c_pos;
            c_pos = (e_pos - s_pos) / 2 + s_pos;
            fseek(fp, c_pos, SEEK_SET);
            fgets(buf, sizeof(buf) - 1, fp); // 取下一行
            continue;
        }
    }

FINAL_FAIL:
    fclose(fp);
    return -1;

FINAL_SUCC:
    fclose(0);
    return 0;
}

void do_it(matcher *ma)
{
    int ret = 0;
    int flag = 0;
    found_pos spos;
    found_pos epos;

    char buf[8192] = {0};
    char *pbuf = buf;

    // Get posision for Start
    if (ma->verbose > 0)
        printf("-----------------------------\n");

    ret = get_start_pos(ma, &spos);
    if (ret == -1)
    {
        printf("Error: get position for start\n");
        return;
    }
    if (ma->verbose > 0)
    {
        printf("[Start] Final: seconds(%llu[%s]), pos(%llu)\n", spos.seconds, spos.time_str, spos.pos);
    }

    // Get position for End
    ret = get_end_pos(ma, spos.pos, &epos);
    if (ret == -1)
    {
        printf("Error: get position for end\n");
        return;
    }
    if (ma->verbose > 0)
    {
        if (ma->end_hour == -1 && ma->end_minute == -1)
        {
            printf("[End] Final: seconds(-1[eof]), pos(%llu)\n", epos.pos);
        }
        else
        {
            printf("[End] Final: seconds(%llu[%s]), pos(%llu)\n", epos.seconds, epos.time_str, epos.pos);
        }
    }

    char size_str[128] = {0};
    kconv_mem_size_to_str((epos.pos - spos.pos), size_str, sizeof(size_str), "%.3f");
    printf("-----------------------------\n");
    if (ma->end_hour == -1 && ma->end_minute == -1)
    {
        printf("Lookup: start[%llu][%s] ---> end[%llu][eof], size[%s]", spos.pos, spos.time_str, epos.pos, size_str);
    }
    else
    {
        printf("Lookup: start[%llu][%s] ---> end[%llu][%s], size[%s]", spos.pos, spos.time_str, epos.pos, epos.time_str, size_str);
    }
    printf("\n-----------------------------\n");

    // search
    FILE *fp = fopen(ma->filename, "r");
    if (fp == NULL)
    {
        printf("Error: open file(%s) fail:%s\n", ma->filename, strerror(errno));
        return;
    }

    fseek(fp, spos.pos, SEEK_SET); // 偏移

    int len = 0;
    int buf_alloc = sizeof(buf);
    unsigned long long total_bytes = (epos.pos - spos.pos);

    while ((total_bytes - len) > 0)
    {
        if (fgets(pbuf, buf_alloc, fp) == NULL)
        {
            // printf("Error: read file(%s) fail:%s\n", ma->filename, strerror(errno));
            fclose(fp);
            return;
        }
        len += strlen(pbuf);

        // 输出
        printf("%s", buf);

        if (ma->debug)
        {
            printf("DEBUG: len/total: %lld/%lld\n", len, total_bytes);
        }
    }

    fclose(fp);
    return;
}

void parse_args(char *type, char *str, matcher *ma)
{
    if (type == NULL)
        return;

    char *sed = NULL;
    if (strcasecmp(type, "start") == 0)
    {
        sed = (char *)memchr(str, ':', strlen(str));
        if (sed == NULL)
        {
            ma->start_hour = atoi(str);
            ma->start_minute = 0;
            return;
        }
        else
        {
            *sed = '\0';
            ma->start_hour = atoi(str);
            ma->start_minute = atoi(sed + 1);
            return;
        }
    }
    else if (strcasecmp(type, "end") == 0)
    {
        sed = (char *)memchr(str, ':', strlen(str));
        if (sed == NULL)
        {
            ma->end_hour = atoi(str);
            ma->end_minute = 0;
            return;
        }
        else
        {
            *sed = '\0';
            ma->end_hour = atoi(str);
            ma->end_minute = atoi(sed + 1);
            return;
        }
    }
}

void usage(char *prog)
{
    printf("--------------------------------------------\n");
    printf("Version: %s\n\n", VERSION);
    printf("SYNOPSIS\n");
    printf("  %s [OPTIONS] PATTERN [FILE]\n", prog);
    printf("\n");
    printf("OPTIONS\n");
    printf("  -d: 几号，默认不写是今天\n");
    printf("  -s: 从指定的开始时间查找，格式:[Hour]:[Minute]，例:(6:12, 6)\n");
    printf("  -e: 查找到指定的结束时间，不写默认到文件结尾\n");
    printf("  -S: 最小size, 单位:MB, 默认:1G\n");
    printf("  -v: 显示详细信息\n");
    printf("  -D: 显示debug\n");
    printf("  -c: 是否有颜色\n");
    printf("\n");
    printf("Usage:\n");
    printf("  %s -v -s15 -e17 [file]\n", prog);
    printf("\n");
    printf("OTHER\n");
    printf("  1. 如果文件是gzip，先解压再查询，如:\n");
    printf("    a. gzip -dvc abc.0.gz > abc.0 \n");
    printf("    b. kgrep -s8 -e10 'kyosold@qq.com' abc.0\n");
    printf("  2. 也可以使用kungz解压文件，把解压出来的写到另一个指定的文件中:\n");
    printf("    a. gcc -g -o kungz kungz.c -lz\n");
    printf("        (注意: 需要用到zlib库)\n");
    printf("    b. ./kungz /var/log/abc.log.0.gz abc\n");
    printf("        说明: 把/var/log/abc.log.0.gz解压的结果写到abc中，原来的abc.log.0.gz保持不变\n");
    printf("--------------------------------------------\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage(argv[0]);
        return 1;
    }

    // Matcher Init
    matcher ma;
    ma.start_hour = -1;
    ma.start_minute = -1;
    ma.day = 0;
    ma.end_hour = -1;
    ma.end_minute = -1;
    ma.min_size = 1073741824; // 1G
    ma.verbose = 0;
    ma.debug = 0;

    // Parase args
    int i, ch;
    const char *args = "s:e:d:S:vDVh";
    while ((ch = getopt(argc, argv, args)) != -1)
    {
        switch (ch)
        {
        case 's':
            parse_args("start", optarg, &ma);
            break;
        case 'e':
            parse_args("end", optarg, &ma);
            break;
        case 'd':
            ma.day = atoi(optarg);
            break;
        case 'S':
            ma.min_size = atoi(optarg) * 1024 * 1024;
            break;
        case 'v':
            ma.verbose = 1;
            break;
        case 'D':
            ma.debug = 1;
            break;
        case 'V':
            printf("Version: %s\n", VERSION);
            exit(0);
        case 'h':
        default:
            usage(argv[0]);
            exit(0);
        }
    }

    if (ma.start_hour == -1)
    {
        ma.start_hour = 0;
        ma.start_minute = 0;
    }

    if (ma.day == 0)
    {
        time_t now = time(NULL);
        ma.day = localtime(&now)->tm_mday;
    }

    if ((argc - optind) != 1)
    {
        usage(argv[0]);
        exit(1);
    }

    snprintf(ma.filename, sizeof(ma.filename) - 1, "%s", argv[optind++]);

    char interval_size_str[128] = {0};
    kconv_mem_size_to_str(ma.min_size, interval_size_str, sizeof(interval_size_str), "%.2f");
    printf("******************************************\n");
    printf("Verbose: %d\n", ma.verbose);
    printf("File: %s\n", ma.filename);
    printf("Interval Size: %llu(%s)\n", ma.min_size, interval_size_str);
    printf("Display Debug: %d\n", ma.debug);
    printf("Day: %d\n", ma.day);
    printf("Start: %d:%d\n", ma.start_hour, ma.start_minute);
    printf("End: %d:%d\n", ma.end_hour, ma.end_minute);
    printf("******************************************\n");

    char *ext = strrchr(ma.filename, '.');
    if (ext && (strcasecmp(ext, ".gz") == 0 || strcasecmp(ext, ".gzip") == 0))
    {
        printf("Error: file(%s) is zip file\n", ma.filename);
        printf("  Run: 'gzip -dvc %s > tmp.log'\n", ma.filename);
        exit(1);
    }
    do_it(&ma);

    return 0;
}
