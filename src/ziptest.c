#include "dzfile-intern.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#define BUFLEN 4 * 1024 * 1024
int
main()
{
  uint64_t i;
  char buf[BUFLEN];
  char* dstdata;
  int dstlen;
  int ret;
  uint64_t elapse;
  struct timeval tv1, tv2;
  time_t tt;
  time(&tt);
  srand(tt);
  int fd = open("/home/chyd/tmp/1.json", O_RDONLY);
  if (fd < 0) {
    printf("open file failed\n");
    return -1;
  }
  ssize_t nbread = read(fd, buf, BUFLEN);
  if (nbread <= 0) {
    printf("read file failed\n");
    close(fd);
    return -1;
  }
  /*
  for (i=0;i<BUFLEN;i++){
      //buf[i] = rand() % 255;
      buf[i] = i % 255;
  }*/
  gettimeofday(&tv1, NULL);
  //ret = compressdata_zstd(&dstdata, &dstlen, buf, nbread);
  ret = compressdata_zlib(&dstdata, &dstlen, buf, nbread);
  if (ret < 0) {
    printf("failed to compress\n");
    return -1;
  }
  gettimeofday(&tv2, NULL);
  elapse = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
  printf("dstlen=%d time=%lld ms\n", dstlen, elapse);
  return 0;
}
