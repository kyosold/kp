#ifndef __K_UTILS_H__
#define __K_TUILS_H__

void kconv_mem_size_to_str(unsigned long long size, char *str, size_t str_size, char *fmt);

void kconv_month_to_str(int month, char *str, size_t str_size, int type);
int kconv_month_to_int(char *month);

#endif