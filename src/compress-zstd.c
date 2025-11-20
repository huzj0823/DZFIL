#include "common.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>

/* srcdata is the original data, dstdata is compressed data
 * return 0 on success, -1 on failure
 * */
int
compressdata_zstd(char** dstdata, int* dstlen, char* srcdata, int srclen)
{
  size_t destLen;
  unsigned char* ostream;
  int ret;
  if (!srcdata || srclen == 0)
    return -1;
  destLen = ZSTD_compressBound(srclen);
  ostream = (unsigned char*)malloc(destLen);
  if (!ostream) {
    dz_msg(DZ_LOG_ERROR, "malloc compress buffer faild!\n");
    return -1;
  }
  /* Here ret is the actual size of compressed data*/
  //ret = ZSTD_compress(ostream, destLen, srcdata, srclen, ZSTD_maxCLevel());
  //ret = ZSTD_compress(ostream, destLen, srcdata, srclen, ZSTD_fast);
  //ret = ZSTD_compress(ostream, destLen, srcdata, srclen, ZSTD_CLEVEL_DEFAULT);  
  ret = ZSTD_compress(ostream, destLen, srcdata, srclen, 6);  
  if (ZSTD_isError(ret)) {
    errno = ENOMEM;
    dz_msg(DZ_LOG_ERROR,
           "Failed to compress data using zstd: %s\n",
           ZSTD_getErrorName(ret));
    free(ostream);
    return -1;
  }
  *dstdata = ostream;
  *dstlen = ret;
  return 0;
}

/* zipdata is the  data, dstdata is compressed data
 * return 0 on success, -1 on failure
 * */

int
decompressdata_zstd(char** dstdata, int* dstlen, char* zipdata, int ziplen)
{
  
  size_t ret = -1;
  char* destdata = NULL;
  uint64_t dSize;
  if (!zipdata || ziplen == 0)
    return -1;
  dSize = ZSTD_getFrameContentSize(zipdata, ziplen);
  if ((dSize == ZSTD_CONTENTSIZE_ERROR) || (dSize == ZSTD_CONTENTSIZE_UNKNOWN)) {
     dz_msg(DZ_LOG_ERROR, "Failed to get the size of decompressed data\n");
    return -1;
  }
  destdata = (char*)malloc(dSize);
  if (!destdata) {
    dz_msg(DZ_LOG_ERROR, "Failed to allocate memory\n");
    return -1;
  }
  bzero(destdata, dSize);
  ret = ZSTD_decompress(destdata, dSize, zipdata, ziplen);
  if (ZSTD_isError(ret)) {
        dz_msg(DZ_LOG_ERROR, "Failed to decompression using zstd: %s\n", ZSTD_getErrorName(ret));
    errno = ENOMEM;
    free(destdata);
    return -1;
  }
  *dstdata = destdata;
  *dstlen = ret;
  return 0;
}
