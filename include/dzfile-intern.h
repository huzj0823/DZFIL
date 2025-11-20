/*dcfile.h for internal*/
#ifndef _DZFILE_INTERN_H
#define _DZFILE_INTERN_H
#include "common.h"

#include <hdf5.h>
#include <sqlite3.h>
#include <stdint.h>
#include <zlib.h>
#include <zstd.h>
#include "compress-fpga.h"

#define H5_READ 1
#define H5_WRITE 2
/*One dataset in H5 File*/
#define H5_BUFSIZE 4194304
// #define H5_BUFSIZE 512
#define ZIP_VERSION 1024
#define DATASET_NAMELEN 255

typedef enum
{
  DZ_ALG_ZLIB,
  DZ_ALG_ZSTD,
  DZ_ALG_ISAL,
  DZ_ALG_FPGA,
  DZ_ALG_UNKOWN,
} dz_alg_t;
typedef struct _fd_map
{
  int fd;               // opened original file
  hid_t h5fd;           // opened compressed file
  const char* filename; // opened filename
  dz_alg_t method;      // compression algorithm
} dzfd_t;

#ifdef __cplusplus
extern "C"
{
#endif

  /*general function to compress data*/
  int compress_data(dz_alg_t method,
                    char** dstdata,
                    int* dstlen,
                    char* srcdata,
                    int srclen);
  /*general function to decompress data*/
  int decompress_data(dz_alg_t method,
                      char** dstdata,
                      int* dstlen,
                      char* zipdata,
                      int ziplen);
  int decompressdata_zlib(char** dstdata,
                          int* dstlen,
                          char* zipdata,
                          int ziplen);

  /* zlib compression */
  int compressdata_zlib(char** dstdata, int* dstlen, char* srcdata, int srclen);
  int decompressdata_zlib(char** dstdata,
                          int* dstlen,
                          char* zipdata,
                          int ziplen);
  /* zstd compression */
  int compressdata_zstd(char** dstdata, int* dstlen, char* srcdata, int srclen);
  int decompressdata_zstd(char** dstdata,
                          int* dstlen,
                          char* zipdata,
                          int ziplen);


  /* Intel ISA-L */

  /*******************************************************************
   * uncompressdata_isa
   *
   * DESCRIPTION:
   *       Intel ISA-L compress api .
   * INPUTS:
   *       srcdata - Data buffer need to be compressed
   *       srclen  - Input data length
   * OUTPUTS:
   *       dstdata - Output data buffer. Dstdata needs to be free manually.
   *       dstlen  - Output data length
   * RETURNS:
   *       0 - on success
   *       -1 - on fail
   * COMMENTS:
   *
   *******************************************************************/
  int compressdata_isa(char** dstdata, int* dstlen, char* srcdata, int srclen);

  /*******************************************************************
   * uncompressdata_isa
   *
   * DESCRIPTION:
   *       Intel ISA-L uncompress api .
   * INPUTS:
   *       zipdata - Data buffer to be decompressed
   *       ziplen  - Input data length
   * OUTPUTS:
   *       dstdata - Output data buffer
   *       dstlen  - Output data length
   * RETURNS:
   *       0 - on success
   *       -1 - on fail
   * COMMENTS:
   *
   *******************************************************************/
  int decompressdata_isa(char** dstdata,
                         int* dstlen,
                         char* zipdata,
                         int ziplen);

  hid_t hdf5_open(const char* path, int flag);
  int hdf5_close(hid_t h5fd);
  ssize_t hdf5_writestr(hid_t h5fd,
                        const char* dataset_name,
                        const char* inputstr);
  char* hdf5_readstr(hid_t h5fd, const char* dataset_name, size_t str_len);
  ssize_t hdf5_writedata(hid_t h5fd,
                         const char* dataset_name,
                         const char* data,
                         size_t datalen);
  ssize_t hdf5_readdata(hid_t h5fd,
                        const char* dataset_name,
                        void* buf,
                        size_t buflen);
  int convert2h5(const char* path, dz_alg_t method);
  dz_alg_t getAlgorithm(hid_t h5fd);
  ssize_t h5fd_read(hid_t h5fd, void* buf, size_t count);
  ssize_t h5fd_pread(hid_t h5fd, void* buf, size_t count, off_t offset);
  /* functions to operate hashmap*/
  void hashmap_init();
  int add_dzfd(int fd, dzfd_t* dzfd);
  int remove_dzfd(int fd);
  dzfd_t* find_dzfd(int fd);
#ifdef __cplusplus
}
#endif

#endif
