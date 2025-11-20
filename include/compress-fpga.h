#ifndef QZ01_COMPRESS_H
#define QZ01_COMPRESS_H

#include <stdint.h>
#include <iostream>
#include "pcie_memio.h" 
#include "srai_accel_utils.h" 

#define BUF_SIZE 4*1024*1024
#define msleep(x) usleep(x*1000)
#define DMA_TX_SEGMENT (4*1024*1024UL)
#define DMA_RX_SEGMENT (128*1024UL)
#define DMA_QUEUE (32)
#define MAX_DMA_TX_SEGMENTS (DMA_TX_SEGMENT*DMA_QUEUE)
#define MAX_DMA_RX_SEGMENTS (DMA_RX_SEGMENT*DMA_QUEUE)

/* FPGA Compression library version */
#define COMPRESSION_FPGA_VERSION "1.0.0"



/* Return codes for the compression/decompression functions. Negative values
 * are errors, positive values are used for special but normal events.
 */
#define F_OK             0
#define F_STREAM_END     1
#define F_ERRNO         (-1)
#define F_STREAM_ERROR  (-2)
#define F_DATA_ERROR    (-3)
#define F_MEM_ERROR     (-4)
#define F_BUF_ERROR     (-5)
#define F_VERSION_ERROR (-6)
#define F_ALGO_ERROR    (-7)
#define F_THREAD_CREATE_ERROR    (-8)
#define F_THREAD_JOIN_ERROR    (-9)


typedef struct {
    char *inbuffer;
    int inbuffer_size;
    fpga_xDMA_linux *fpga_xDMA_ptr;

}submit_param_t;

typedef struct {
    char *outbuffer;
    int* outbuffer_size;
    fpga_xDMA_linux *fpga_xDMA_ptr;
}cmplt_param_t;



int compressdata_fpga(char **dstdata, int *dstlen, char *srcdata, int srclen);
int decompressdata_fpga_gzip(unsigned char* uncompressed_data, uint64_t &uncompressed_size, unsigned char* compressed_data, uint64_t compressed_size);
const char* compression_fpga_version(void);



#endif // QZ01_COMPRESS_H
