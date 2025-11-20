#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "dzfile-api.h"
#include "dzfile.h"

#define BUFLEN 524288
#define SIXMONTHS (6 * 30 * 24 * 60 * 60)
int helpflg;
int verbose;
int errflg;
int lflag;
int iflag;
time_t current_time;

int
main(int argc, char* argv[])
{
  int ret;
  int c;
  char buf[BUFLEN];
  int fd;
  int dstfd;
  int i;
  int nbread = 0;
  int nbwrite = 0;
  int nbtotal = 0;
  struct stat stbuf;
  struct statvfs fsbuf;
  struct dzfile_dinfo dbuf;
  char* realpath = NULL;
  char* meta_path = NULL;
  char* data_path = NULL;
  char* src_file = NULL;
  char* filename = NULL;
  DIR* dirp = NULL;
  struct dirent* dentp = NULL;

  static struct option longopts[] = { { "help", no_argument, &helpflg, 1 },
                                      { "verbose", no_argument, &verbose, 1 },
                                      { 0, 0, 0, 0 } };

  while ((c = getopt_long(argc, argv, "hm:c:vlid:", longopts, NULL)) != EOF) {
    switch (c) {
      case 'h':
        helpflg = 1;
        break;
      case 'd':
        data_path = optarg;
      case 'v':
        verbose = 1;
        break;
      case 'l':
        lflag = 1;
        break;
      case 'i':
        iflag = 1;
        break;
      case '?':
        errflg++;
        break;
      default:
        break;
    }
  }
  if (errflg || helpflg) {
    printf("usage: %s %s %s\n",
           argv[0],
           "-c data_path [default: /home/chyd/dzfile] ",
           "[-l] [filename]");
    printf("\tfor example %s /hosts\n", argv[0]);
    return -1;
  }
  (void)time(&current_time);

  if (optind >= argc) {
    filename = "/";
  } else {
    filename = argv[optind];
  }
  if (data_path == NULL) {
    data_path = "/home/chyd/dzfile";
  }
  src_file = (char*)malloc(strlen(data_path) + strlen(filename) + 2);
  if (!src_file) {
    printf("Failed to allocate memory for src_file %s %s\n",
           filename,
           strerror(errno));
    return (-1);
  }
  sprintf(src_file, "%s/%s", data_path, filename);

  if (verbose) {
    printf("DEBUG: data_path=%s file=%s\n", data_path, src_file);
  }

  ret = dzfile_init(data_path, NULL);
  if (ret < 0) {
    if (src_file) {
      free(src_file);
    }
    return -1;
  }
  ret = dzfile_dstat(src_file, &dbuf);
  if (ret < 0) {
    printf("dstat file '%s' failed %s\n", src_file, strerror(errno));
    goto out;
  }
  printf("## DZFILE overview ##\n");
  // printf("Files        \t\t%lld\n", dbuf.filenum);
  printf("Total Capacity of FS\t\t%llu KB\n", dbuf.capacity / 1024);
  printf("Used Space of FS\t\t%llu KB\n", dbuf.usedspace / 1024);
  printf("Files        \t\t\t%llu\n", dbuf.filenum);
  printf("Total Data Size \t\t%llu Bytes\n", dbuf.totalsize);
  printf("Acutal Disk Consumption \t%llu Bytes\n", dbuf.disksize);
  printf("Compression Ratio   \t\t%.2f\n", dbuf.comratio);
  if (dbuf.totalsize > dbuf.disksize) {
    uint64_t saved_bytes = dbuf.totalsize - dbuf.disksize;
    printf("Saved Disk Space \t\t%lld\n", saved_bytes);
    printf("Saved Disk Ratio\t\t%.2f%\n", (double) (saved_bytes * 100) / dbuf.totalsize);
  }
   
out:
  if (src_file) {
    free(src_file);
  }
  dzfile_fini();
  return ret;
}
