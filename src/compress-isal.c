#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "igzip_lib.h"
#include "common.h"

#define BUF_SIZE 4*1024*1024

#define LEVEL_DEFAULT    2
#define UNIX            3

int level_size_buf[10] = {
#ifdef ISAL_DEF_LVL0_DEFAULT
    ISAL_DEF_LVL0_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL1_DEFAULT
    ISAL_DEF_LVL1_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL2_DEFAULT
    ISAL_DEF_LVL2_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL3_DEFAULT
    ISAL_DEF_LVL3_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL4_DEFAULT
    ISAL_DEF_LVL4_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL5_DEFAULT
    ISAL_DEF_LVL5_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL6_DEFAULT
    ISAL_DEF_LVL6_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL7_DEFAULT
    ISAL_DEF_LVL7_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL8_DEFAULT
    ISAL_DEF_LVL8_DEFAULT,
#else
    0,
#endif
#ifdef ISAL_DEF_LVL9_DEFAULT
    ISAL_DEF_LVL9_DEFAULT,
#else
    0,
#endif
};


int compressdata_isa(char **dstdata, int *dstlen, char *srcdata, int srclen)
{
    struct isal_zstream stream;
    struct isal_gzip_header gz_hdr;
    unsigned char *outbuf = NULL, *level_buf = NULL;    //*inbuf = NULL,
    size_t inbuf_size = BUF_SIZE, outbuf_size = BUF_SIZE*2;
    size_t level_buf_size;
    int ret = -1;

    isal_gzip_header_init(&gz_hdr);
    gz_hdr.os = UNIX;
    //gz_hdr.name_buf_len = infile_name_len + 1;
    
    level_buf_size = level_size_buf[LEVEL_DEFAULT];
    //inbuf = malloc (inbuf_size);
    outbuf = malloc (outbuf_size);
    level_buf =  malloc (level_buf_size);
    if (!outbuf || !level_buf) {
        dz_msg(DC_LOG_ERROR, "Malloc inbuf, outbuf failed \n");
        goto ERR;
    }
    
    isal_deflate_init(&stream);
    stream.avail_in = 0;
    stream.flush = NO_FLUSH;
    stream.level = LEVEL_DEFAULT;
    stream.level_buf = level_buf;
    stream.level_buf_size = level_buf_size;
    stream.gzip_flag = IGZIP_GZIP_NO_HDR;
    stream.next_out = outbuf;
    stream.avail_out = outbuf_size;
    
    /* Fill the file header with 10 bytes of compressed information */
    isal_write_gzip_header(&stream, &gz_hdr);
    
    stream.next_in = srcdata;
    stream.avail_in = srclen;
    stream.end_of_stream = 1;        // 1 yes  0 no
    
    if (stream.next_out == NULL) {
        stream.next_out = outbuf;
        stream.avail_out = outbuf_size;
    }

    ret = isal_deflate(&stream);
    if (ret != ISAL_DECOMP_OK) {
        dz_msg(DC_LOG_ERROR, "igzip error encountered while compressing file .\n");
        goto ERR;
    }
    
    *dstdata = outbuf;
    *dstlen = stream.next_out - outbuf;
    
    if (level_buf) {
        free (level_buf);
    }
    
    return 0;
    
ERR:
    if (outbuf) {
        free (outbuf);
    }
    if (level_buf) {
        free (level_buf);
    }
    
    return -1;
}

int decompressdata_isa(char **dstdata, int *dstlen, char *zipdata, int ziplen)
{
    struct inflate_state state;
    struct isal_gzip_header gz_hdr;
    FILE *in = NULL, *out = NULL;
    unsigned char *outbuf = NULL;         // *inbuf = NULL,
    size_t outbuf_size = BUF_SIZE*2;     // inbuf_size = BUF_SIZE,
    size_t destlen = 0;
    int ret = -1;
    

    outbuf = malloc (outbuf_size);
    if (!outbuf) {
        dz_msg(DC_LOG_ERROR, "Malloc outbuf failed .\n");
        return -1;
    }
    
    isal_gzip_header_init(&gz_hdr);
    
    isal_inflate_init(&state);
    state.crc_flag = ISAL_GZIP_NO_HDR_VER;
    state.next_in = zipdata;
    state.avail_in = ziplen;

    /* Read and parse the first 10 bytes of compressed information in the file header */
    ret = isal_read_gzip_header(&state, &gz_hdr);
    if (ret != ISAL_DECOMP_OK) {
        dz_msg(DC_LOG_ERROR,"igzip: Error invalid gzip header found for file .\n");
        goto ERR;
    }
    
    //state.next_in = inbuf;
    //state.avail_in =
    state.next_out = outbuf;
    state.avail_out = outbuf_size;
    
    ret = isal_inflate(&state);
    if (ret != ISAL_DECOMP_OK) {
        dz_msg(DC_LOG_ERROR, "igzip: Error encountered while decompressing file.\n");
        goto ERR;
    }
    destlen = state.next_out - outbuf;
    *dstlen = destlen;
    memcpy(*dstdata, outbuf, destlen);
    
    
    if (outbuf) {
        free(outbuf);
    }
    return 0;
ERR:
    if (outbuf) {
        free(outbuf);
    }
    return -1;
}

