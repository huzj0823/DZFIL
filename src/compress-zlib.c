#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"

/* srcdata is the original data, dstdata is compressed data
 * return 0 on success, -1 on failure
 * */
int compressdata_zlib(char **dstdata, int *dstlen, char *srcdata, int srclen)
{
    uLong            destLen;
    Bytef           *ostream;
    int              ret;
    
    destLen = compressBound(srclen);
    ostream = (unsigned char*)malloc(destLen);
    if (!ostream) {
        return -1;
    }
    ret = compress(ostream, &destLen, (const Bytef*)srcdata, (uLong)srclen);
    if (ret == Z_BUF_ERROR) {
        errno = ENOMEM;
        dz_msg(DZ_LOG_ERROR, "Buffer was too small!\n");
        free(ostream);
        return -1;
    }
    if (ret == Z_MEM_ERROR) {
        errno = ENOMEM;
        dz_msg(DZ_LOG_ERROR, "Not enough memory for compression!\n");
        free(ostream);
        return -1;
    }
    if (ret != Z_OK) {
        dz_msg(DZ_LOG_ERROR, "compression failed!\n");
        free(ostream);
        return -1;
    }
    *dstdata = ostream;
    *dstlen = destLen;
    return 0;
}

/* zipdata is the  data, dstdata is compressed data
 * return 0 on success, -1 on failure
 * */

int decompressdata_zlib(char **dstdata, int *dstlen, char *zipdata, int ziplen)
{
    uLongf      destlen = (uLongf) * dstlen;
    int         ilen = *dstlen;
    int         ret = -1;
    char       *destdata  = NULL;
    
    if (ilen <= 0) {
        errno = EINVAL;
        return -1;
    }
    destdata = (char *)malloc(ilen);
    if (!destdata) {
        dz_msg(DZ_LOG_ERROR, "failed to allocate memory\n");
        return -1;
    }
    bzero(destdata, ilen);
    ret = uncompress(destdata, &destlen, zipdata, ziplen);
    switch (ret) {
    case Z_MEM_ERROR:
        errno = ENOMEM;
        dz_msg(DZ_LOG_ERROR, "there was no enough memory\n");
        goto ERR;
    case Z_BUF_ERROR:
        errno = ENOMEM;
        dz_msg(DZ_LOG_ERROR, "there was not enough room in the output buffer\n");
        goto ERR;
    case Z_DATA_ERROR:
        errno = EINVAL;
        dz_msg(DZ_LOG_ERROR, "the input data was corrupted or incomplete, destlen %d, ziplen %d\n",
               destlen, ziplen);
        goto ERR;
    case Z_OK:
        //printf("%s uncompression successfully\n", __FUNCTION__);
        break;
    default:
        dz_msg(DZ_LOG_ERROR, "uncompressed failed destlen %d ziplen %d data ='%s'\n",
               destlen, ziplen, destdata);
        goto ERR;
    }
    memcpy(*dstdata, destdata, destlen);
    *dstlen = destlen;
    if (destdata) {
        free(destdata);
    }
    return 0;
ERR:
    if (destdata) {
        free(destdata);
    }
    return -1;
}

