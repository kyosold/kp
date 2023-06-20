#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "klog.h"

static int klog_level = KLOG_INFO;
static char klog_sid[128] = {0};
static char klog_ident[512] = {0};
static char klog_file[1024] = {0};
static char *klog_priority[] = {
    "EMERG",
    "ALERT",
    "CRIT",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG"
};

#if defined(WIN32) || defined(_WIN32) 
#else
static int klog_facility = LOG_USER;
static int klog_opt = LOG_PID | LOG_NDELAY;

void _klog_write(int level, const char *file, int line,
                const char *fun, const char *fmt, ...)
{
    if (level > klog_level)
        return;

    int n;
    char new_fmt[KLOG_MAX_LINE];
    if (klog_sid[0] != '\0') {
        const char *head_fmt = "[%s] %s:%d:%s %s %s";
        snprintf(new_fmt, sizeof(new_fmt), head_fmt, klog_priority[level], file, line, fun, klog_sid, fmt);
    } else {
        const char *head_fmt = "[%s] %s:%d:%s %s";
        snprintf(new_fmt, sizeof(new_fmt), head_fmt, klog_priority[level], file, line, fun, fmt);
    }

    va_list ap;
    va_start(ap, fmt);
    openlog(klog_ident, klog_opt, klog_facility);
    vsyslog(level, new_fmt, ap);
    closelog();
    va_end(ap);
}

/**
 * @brief open log
 * 
 * @param ident     the name of program
 * @param facility  LOG_MAIL/LOG_USER
 * @param level     KLOG_DEBUG/KLOG_INFO ...
 * @param sid       using tag session
 */
void klog_open(const char *ident, int facility, int level, char *sid)
{
    snprintf(klog_ident, sizeof(klog_ident), "%s", ident);
    klog_facility = facility;
    klog_level = level;
    if (sid == NULL)
        memset(klog_sid, 0, sizeof(klog_sid));
    else
        snprintf(klog_sid, sizeof(klog_sid), "%s", sid);
}

#endif





void _klog_write_file(int level, const char *fname, int line,
                      const char *fun, const char *fmt, ...)
{
    if (level > klog_level)
        return;

    FILE *fp;
    time_t local_time;
    char time_str[128];
    char klog_line_str[KLOG_MAX_LINE];

    // 获取本地时间
    time(&local_time);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&local_time));

    // 日志内容格式转换
    char new_fmt[KLOG_MAX_LINE];
    const char *head_fmt = "[%s] %s:%d:%s %s %s";
    snprintf(new_fmt, sizeof(new_fmt), head_fmt, klog_priority[level], fname, line, fun, klog_sid, fmt);

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(klog_line_str, sizeof(klog_line_str), fmt, ap);
    va_end(ap);

    // 打开日志
    fp = fopen(klog_file, "a");
    if (fp != NULL) {
        fprintf(fp, "%s [%s:%d:%s]: [%s] sid:%s %s\n",
                time_str, fname, line, fun, klog_priority[level], klog_sid, klog_line_str);
        fclose(fp);
    }
}



void klog_open_file(const char *file, const char *ident, int level, char *sid)
{
    snprintf(klog_file, sizeof(klog_file), "%s", file);
    snprintf(klog_ident, sizeof(klog_ident), "%s", ident);
    klog_level = level;
    snprintf(klog_sid, sizeof(klog_sid), "%s", sid);
}

/**
 * @brief set log level
 * 
 * @param level     KLOG_DEBUG/KLOG_INFO ...
 */
void klog_set_level(int level)
{
    klog_level = level;
}
/**
 * @brief set log session id
 * 
 * @param sid       using tag session
 */
void klog_set_sid(char *sid)
{
    if (sid != NULL)
        snprintf(klog_sid, sizeof(klog_sid), "%s", sid);
}

