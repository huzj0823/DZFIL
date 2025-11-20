#include "compress-fpga.h"
#include <pthread.h>
#include <stdio.h>
#include <zlib.h>
#include <stdlib.h>




static void *thread_send(void *arg);
static void *thread_recv(void *arg);


int decompressdata_fpga_gzip(unsigned char* uncompressed_data, uint64_t &uncompressed_size, unsigned char* compressed_data, uint64_t compressed_size) {
// int decompressdata_fpga_gzip(unsigned char* uncompressed_data, uint64_t &uncompressed_size, unsigned char* compressed_data, uint64_t compressed_size) {
    z_stream zs;                        
    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, 16+MAX_WBITS) != Z_OK) {
        std::cerr << "inflateInit2 failed while decompressing." << std::endl;
        return -1;
    }

    zs.next_in = (Bytef *)compressed_data;
    zs.avail_in = (uInt)compressed_size;
    zs.next_out = (Bytef *)uncompressed_data;
    zs.avail_out = (uInt)uncompressed_size;

    int ret = inflate(&zs, Z_FINISH);
    if (ret != Z_STREAM_END) {
        inflateEnd(&zs);
        std::cerr << "Failed to decompress the data: " << ret << std::endl;
        return -1;
    }
    uncompressed_size = uncompressed_size - zs.avail_out; 

    inflateEnd(&zs);
}


int compressdata_fpga(char **dstdata, int *dstlen, char *srcdata, int srclen) {
// int compressdata_fpga(unsigned char* dest, uint64_t *destLen, unsigned char* source, uint64_t sourceLen) {

    pthread_t send_thread, recv_thread;
    fpga_xDMA_linux *fpga_xDMA_ptr;
    submit_param_t submit_param;
    cmplt_param_t cmplt_param;

    char *outbuf = NULL, *level_buf = NULL;  
    int outbuf_size = BUF_SIZE*2;  

    /* Initializing XDMA */
    fpga_xDMA_ptr = new fpga_xDMA_linux;
    fpga_xDMA_ptr -> fpga_xDMA_init();

    outbuf = (char *)malloc(outbuf_size);   // ! Released by the caller
    if (!outbuf) {
        return F_BUF_ERROR;
    }

    /* Reset Compress Statistics Engine */
    fpga_run_NORTH_PR64(fpga_xDMA_ptr, NULL, NULL, srclen);

    /* Create compress submit thread */
    submit_param.inbuffer = srcdata;
    submit_param.inbuffer_size = srclen;
    submit_param.fpga_xDMA_ptr = fpga_xDMA_ptr;
    if(pthread_create(&send_thread, NULL, thread_send, &submit_param)) {
	    printf("error creating submit thread.\r\n");
        return F_THREAD_CREATE_ERROR;
    }

    /* Create compress cmplt thread */
    cmplt_param.outbuffer = outbuf;
    cmplt_param.outbuffer_size = &outbuf_size;
    cmplt_param.fpga_xDMA_ptr = fpga_xDMA_ptr;
    if(pthread_create(&recv_thread, NULL, thread_recv, &cmplt_param)) {
	    printf("error creating cmplt thread.\r\n");
        return F_THREAD_CREATE_ERROR;
    }   

    /* Destroy compress submit thread */
    if(pthread_join(send_thread, NULL)) {
	    printf("error joining sumbmit thread.\r\n");
        return F_THREAD_JOIN_ERROR;
    }

    /* Destroy compress cmplt thread */
    if(pthread_join(recv_thread, NULL)) {
	    printf("error joining cmplt thread.\r\n");
        return F_THREAD_JOIN_ERROR;
    }

    *dstdata = (char *)outbuf;
    *dstlen = outbuf_size;

    fpga_clean(fpga_xDMA_ptr);
    return F_OK;
}


// Sending thread
static void *thread_send(void *arg) {
    submit_param_t *submit_param = (submit_param_t *)arg;
    int inbuffer_size = submit_param -> inbuffer_size;

    if(inbuffer_size > 0) {
        /* Xfer input data to Compress Engine */ 
        fpga_xfer_data_to_card64(submit_param -> fpga_xDMA_ptr, NULL, submit_param -> inbuffer, inbuffer_size);
    }

    return ((void *)0);
}


// receiving thread
static void *thread_recv(void *arg) {
    cmplt_param_t *cmplt_param = (cmplt_param_t *)arg;
    bool done_flag = false;

    while(1) {
	    /* Wait Compress Engine completed */
        done_flag = fpga_check_compute_done_NORTH_PR(cmplt_param -> fpga_xDMA_ptr) ? true : false;        
        *(cmplt_param -> outbuffer_size) = fpga_check_completion_bytes_NORTH_PR(cmplt_param -> fpga_xDMA_ptr);

	    if(done_flag == true) {
            /* Xfer output data to buffer */ 
            fpga_xfer_data_from_card64(cmplt_param -> fpga_xDMA_ptr, NULL, (char*)(cmplt_param -> outbuffer ), *(cmplt_param->outbuffer_size));
            break;
        }
    }

    return ((void *)0);
}

const char* compression_fpga_version(void) {
    return COMPRESSION_FPGA_VERSION;
}
