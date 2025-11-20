#ifndef __SQLITE_IFCE_H__
#define __SQLITE_IFCE_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#define SUB_NUM 0
#define ADD_NUM 1

struct space_entry {
    ssize_t writtenbytes;
    ssize_t diskbytes;
    uint64_t filenum;
};

int dz_opendb();
int dz_closedb();
int fmt_entry_insert(const char* path);
int fmt_entry_delete(const char* path);
int add_writtenbytes(ssize_t nbytes);
int sub_writtenbytes(ssize_t nbytes);
int add_diskbytes(ssize_t nbytes);
int sub_diskbytes(ssize_t nbytes);
int add_filenum(ssize_t num);
int sub_filenum(ssize_t num);
int get_space_entry(struct space_entry* spentry);

#endif // 
