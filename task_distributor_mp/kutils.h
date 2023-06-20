#ifndef __KUTILS_H__
#define __KUTILS_H__

#include <sys/stat.h>
#include <sys/types.h>

int kmkdirs(char *pathname, mode_t mode);
void kgen_uuid(char *uuid, size_t uuid_size, int isUpper);
int kis_file_exists(char *filename);
int kwrite_fail_queue(char *buf, char *dir, char *fid);

unsigned int kscan_ulong(char *s, unsigned long *u);
unsigned int kfmt_ulong(unsigned long u, char *s);

#endif