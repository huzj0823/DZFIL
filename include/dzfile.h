/*dcfile.h for internal*/
#ifndef _DZFILE_H
#define _DZFILE_H
#include "common.h"

#include <sqlite3.h>
#include <stdint.h>

#ifndef _XOPEN_PATH_MAX
#define _XOPEN_PATH_MAX 1024
#endif

struct dzfile_priv
{
  int32_t path_max;
  char* data_path;

  dz_loglevel_t loglevel;
  int logfd;     // opened fd for loging
  char* logfile; // opened fd for loging

  void* mqcontext; // ZeroMQ context
  void* mqsocket;  // ZeroMQ socket
  sqlite3* db;
  dz_lock_t dblock;
  int shutdown;
};

struct dzfile_dinfo
{
  uint64_t capacity;  /* Total capacity of one filesystem */
  uint64_t usedspace; /* Used capacity of one filesystem */
  uint64_t f_files;   /* Total file nodes in filesystem */
  uint64_t filenum;   /* Total file number in DZFile */
  uint64_t totalsize; /* The totalsize of one file or fs */
  uint64_t disksize;  /* The bytes stored in disk by one file or fs */
  float comratio;     /* compression ratio of one file or fs*/
};

int
dzfile_dstat(const char* path, struct dzfile_dinfo* dbuf);

struct dzfile_priv*
get_priv();
int
dz_openlog(const char* logfile);
#endif
