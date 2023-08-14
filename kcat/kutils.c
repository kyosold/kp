#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/**
 * @brief 转换内存大小的单位
 *
 * @param size  内存大小，单位: 字节
 * @param str   保存转换后的结果
 * @param str_size  str的内存大小
 * @param fmt   设置需要转换的格式，默认:%.2f(设置NULL)
 *
 * @example
 * unsigned long long mem = 928178401;
 * char mem_str[1024];
 * kconv_mem_size_to_str(mem, mem_str, sizeof(mem_str), "%.3f");
 * mem_str: 885.180MB
 */
void kconv_mem_size_to_str(unsigned long long size, char *str, size_t str_size, char *fmt)
{
    unsigned long long GB = 1073741824; // 1024 * 1024 * 1024
    unsigned long long MB = 1048576;    // 1024 * 1024
    unsigned long long KB = 1024;
    char f[128] = {0};
    float res = 0.0;

    if (fmt == NULL) {
        strcpy(f, "%.2f");
    } else {
        snprintf(f, sizeof(f) - 1, "%s", fmt);
    }
    fmt = f;

    if (size > GB)
    {
        res = (float)size / GB;
        strcat(f, "GB");
    }
    else if (size > MB)
    {
        res = (float)size / MB;
        strcat(f, "MB");
    }
    else if (size > KB)
    {
        res = (float)size / KB;
        strcat(f, "KB");
    }
    else
    {
        res = (float)size;
        strcat(f, "Byte");
    }
    snprintf(str, str_size - 1, fmt, res);
}

/**
 * @brief 转换数字月份到字符缩写
 * 
 * @param month     被转换的月份, 如: 5, 6, 12
 * @param str       转换的结果, 如: May, Jun, Dec
 * @param str_size  str的内存大小
 * @param type      转换后的大小写: 1(全小写) 2(全大写) 0(第1个字符大写，其它小写)
 */
void kconv_month_to_str(int month, char *str, size_t str_size, int type)
{
    switch (month)
    {
    case 1:
        snprintf(str, str_size, "Jan");
        break;
    case 2:
        snprintf(str, str_size, "Feb");
        break;
    case 3:
        snprintf(str, str_size, "Mar");
        break;
    case 4:
        snprintf(str, str_size, "Apr");
        break;
    case 5:
        snprintf(str, str_size, "May");
        break;
    case 6:
        snprintf(str, str_size, "Jun");
        break;
    case 7:
        snprintf(str, str_size, "Jul");
        break;
    case 8:
        snprintf(str, str_size, "Aug");
        break;
    case 9:
        snprintf(str, str_size, "Sep");
        break;
    case 10:
        snprintf(str, str_size, "Oct");
        break;
    case 11:
        snprintf(str, str_size, "Nov");
        break;
    case 12:
        snprintf(str, str_size, "Dec");
        break;
    default:
        snprintf(str, str_size, "Unk");
        break;
    }
    if (type == 1) {    // 转小写
        str[1] = tolower(str[1]);
    } else if (type == 2) { // 转大写
        str[2] = toupper(str[2]);
        str[3] = toupper(str[3]);
    }
}

/**
 * @brief 转换月份缩写到整形
 * 
 * @param month     被转换的月份的缩写, 如: Jan, May
 * @return int      返回被转换后的结果，如果出错返回-1
 */
int kconv_month_to_int(char *month)
{
    if (month == NULL)
        return -1;

    char str[4];
    str[0] = tolower(*month);
    str[1] = tolower(*(month + 1));
    str[2] = tolower(*(month + 2));
    str[3] = '\0';

    if (strcmp(str, "jan") == 0) {
        return 1;
    } else if (strcmp(str, "feb") == 0) {
        return 2;
    } else if (strcmp(str, "mar") == 0) {
        return 3;
    } else if (strcmp(str, "apr") == 0) {
        return 4;
    } else if (strcmp(str, "may") == 0) {
        return 5;
    } else if (strcmp(str, "jun") == 0) {
        return 6;
    } else if (strcmp(str, "jul") == 0) {
        return 7;
    } else if (strcmp(str, "aug") == 0) {
        return 8;
    } else if (strcmp(str, "sep") == 0) {
        return 9;
    } else if (strcmp(str, "oct") == 0) {
        return 10;
    } else if (strcmp(str, "nov") == 0) {
        return 11;
    } else if (strcmp(str, "dec") == 0) {
        return 12;
    } else {
        return -1;
    }
}
