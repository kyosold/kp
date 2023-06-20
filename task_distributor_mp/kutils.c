#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <ctype.h>
#include "klog.h"
#include "kutils.h"

/**
 * @brief 一级一级创建目录，如果存在就忽略
 * 
 * @param pathname The directory path
 * @param mode  The permissions to use
 * @return int  Returns zero on success, or -1 if an error occurred (in which case, errno is set appropriately).
 */
int kmkdirs(char *pathname, mode_t mode)
{
    char dir_name[4096] = {0};
    strcpy(dir_name, pathname);

    int i, len = strlen(dir_name);
    if (dir_name[len - 1] != '/')
        strcat(dir_name, "/");

    for (i = 1; i < len; i++) {
        if (dir_name[i] == '/') {
            dir_name[i] = '\0';
            if (access(dir_name, F_OK) != 0) {
                umask(0);
                if (mkdir(dir_name, mode) == -1) {
                    return 1;
                }
            }
            dir_name[i] = '/';
        }
    }
    return 0;
}

/**
 * @brief Checks whether a file or directory exists
 *
 * @param filename
 * @return int Return 0 if the file doesn't exists; 1 is exists.
 */
int kis_file_exists(char *filename)
{
    if (filename == NULL)
        return 0;

    if (access(filename, F_OK) != 0)
        return 0;
    else
        return 1;
}

void kgen_uuid(char *uuid, size_t uuid_size, int isUpper)
{
    memset(uuid, 0, uuid_size);

    uuid_t u;
    uuid_generate(u);
    unsigned char *p = u;
    int i = 0;
    char ch[5] = {0};
    for (i = 0; i < sizeof(uuid_t); i++,p++) {
        if (isUpper)
            snprintf(ch, sizeof(ch), "%02X", *p);
        else
            snprintf(ch, sizeof(ch), "%02x", *p);
        uuid[i * 2] = ch[0];
        uuid[i * 2 + 1] = ch[1];
    }
}

/**
 * @brief 转换月份从字符到数字
 * 
 * @param month         月份缩写
 * @param numstr 
 * @param numstr_size 
 * @return int Returns zero on success, or -1 if an error occurred
 */
int kconv_month_str2numstr(char *month, char *numstr, size_t numstr_size)
{
    if (month == NULL)
        snprintf(numstr, numstr_size, "00");
    else {
        if (strcasecmp(month, "jan") == 0)
            snprintf(numstr, numstr_size, "01");
        else if (strcasecmp(month, "feb") == 0)
            snprintf(numstr, numstr_size, "02");
        else if (strcasecmp(month, "mar") == 0)
            snprintf(numstr, numstr_size, "03");
        else if (strcasecmp(month, "apr") == 0)
            snprintf(numstr, numstr_size, "04");
        else if (strcasecmp(month, "may") == 0)
            snprintf(numstr, numstr_size, "05");
        else if (strcasecmp(month, "jun") == 0)
            snprintf(numstr, numstr_size, "06");
        else if (strcasecmp(month, "jul") == 0)
            snprintf(numstr, numstr_size, "07");
        else if (strcasecmp(month, "aug") == 0)
            snprintf(numstr, numstr_size, "08");
        else if (strcasecmp(month, "sep") == 0)
            snprintf(numstr, numstr_size, "09");
        else if (strcasecmp(month, "oct") == 0)
            snprintf(numstr, numstr_size, "10");
        else if (strcasecmp(month, "nov") == 0)
            snprintf(numstr, numstr_size, "11");
        else if (strcasecmp(month, "dec") == 0)
            snprintf(numstr, numstr_size, "12");
        else
            return 1;
    }
    return 0;
}

/**
 * @brief Write task buf to queue of failed
 * 
 * @param buf 
 * @param dir   The directory for store failed task
 * @return int  Return 0 is success or 1 is fail.
 */
int kwrite_fail_queue(char *buf, char *dir, char *fid)
{
    int i = 0;
    char file_name[4096] = {0};

    if (*(dir + strlen(dir) - 1) == '/')
        *(dir + strlen(dir) - 1) = '\0';

    while (1) {
        if (i == 0) {
            snprintf(file_name, sizeof(file_name), "%s/%s", dir, fid);
        } else {
            snprintf(file_name, sizeof(file_name), "%s/%s.%d", dir, fid, i);
        }
        if (kis_file_exists(file_name) == 0) {
            break;
        }
        i++;
    }

    FILE *fp = fopen(file_name, "w+");
    if (fp == NULL) {
        klog_error("fopen file(%s) fail:%m", file_name);
        return 1;
    }
    fputs(buf, fp);
    fclose(fp);

    klog_debug("write buf:[%d]%s", strlen(buf), buf);
    klog_debug("fail queue file:%s", file_name);

    return 0;
}

/**
 * @brief 转换字符串数字到ulong数字 ("1893147" -> 1893147)
 * 
 * @param s 
 * @param u 
 * @return unsigned int 
 */
unsigned int kscan_ulong(char *s, unsigned long *u)
{
    unsigned int pos = 0;
    unsigned long result = 0;
    unsigned long c;
    while ((c = (unsigned long)(unsigned char)(s[pos] - '0')) < 10)
    {
        result = result * 10 + c;
        ++pos;
    }
    *u = result;
    return pos;
}
/**
 * @brief 转换ulong数字到字符串数字 (1893147 -> "1893147")
 * 
 * @param u 
 * @param s 
 * @return unsigned int 
 */
unsigned int kfmt_ulong(unsigned long u, char *s)
{
    unsigned int len = 1;
    unsigned long q = u;
    while (q > 9)
    {
        ++len;
        q /= 10;
    }
    if (s)
    {
        s += len;
        do
        {
            *--s = '0' + (u % 10);
            u /= 10;
        } while (u);
    }
    return len;
}
