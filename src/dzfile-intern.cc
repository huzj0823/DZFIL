#include "dzfile-intern.h"
#include "dzfile.h"
#include <fcntl.h>
#include <hdf5.h>
#include <sqlite-ifce.h>
#include <string.h>
#include <sys/stat.h>
#include <zmq.h>

hid_t
hdf5_open(const char* path, int flag)
{
  hid_t h5fd = -1;
  if (!path)
    return -1;
  if (flag == H5_READ) {
    h5fd = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT);
  }
  if (flag == H5_WRITE) {
    h5fd = H5Fcreate(path, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  }
  if (h5fd <= 0) {
    dz_msg(DZ_LOG_ERROR, "open HDF5 file '%s' failed\n", path);
    return -1;
  }
  return h5fd;
}
int
hdf5_close(hid_t h5fd)
{
  return H5Fclose(h5fd);
}
/*
Function: hdf5_writestr
This function writes a string to an HDF5 file with the given file handle.
h5fd: the HDF5 file handle to write to
inputstr: the string to be written to the HDF5 file
Returns: the number of bytes written if successful, -1 if h5fd is invalid or
inputstr is NULL
*/
ssize_t
hdf5_writestr(hid_t h5fd, const char* dataset_name, const char* inputstr)
{
  if (h5fd <= 0 || !inputstr || !dataset_name)
    return -1;
  ssize_t ret;
  hid_t str_dataspace = H5Screate(H5S_SCALAR); // create scalar dataspace
  hid_t str_datatype = H5Tcopy(H5T_C_S1);      // create datatype for char array
  H5Tset_size(str_datatype, strlen(inputstr)); // set datatype size
  hid_t str_dataset = H5Dcreate(h5fd, // create dataset in the HDF5 file
                                dataset_name,
                                str_datatype,
                                str_dataspace,
                                H5P_DEFAULT,
                                H5P_DEFAULT,
                                H5P_DEFAULT);
  ret = H5Dwrite(str_dataset,
                 str_datatype,
                 H5S_ALL,
                 H5S_ALL,
                 H5P_DEFAULT,
                 inputstr); // write the data to the dataset
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "write hdf5 file failed");
    return -1;
  }
  H5Tclose(str_datatype);  // close datatype
  H5Sclose(str_dataspace); // close dataspace
  H5Dclose(str_dataset);   // close dataset
  return ret;
}
/*
 * Function: hdf5_readstr
 * ----------------------
 * Reads a string from an HDF5 dataset and returns it in a newly allocated
 * buffer.
 *
 * h5fd: HDF5 file identifier
 * dataset_name: Name of the dataset containing the string
 * str_len: Maximum length of the string to read
 *
 * returns: A pointer to the allocated string buffer if successful, NULL
 * otherwise
 */
char*
hdf5_readstr(hid_t h5fd, const char* dataset_name, size_t str_len)
{
  ssize_t ret;
  if (h5fd <= 0 || !dataset_name || str_len < 1)
    return NULL;

  // Open the dataset
  hid_t dataset = H5Dopen2(h5fd, dataset_name, H5P_DEFAULT);
  // Get the datatype of the dataset
  hid_t datatype = H5Dget_type(dataset);
  int len = H5Tget_size(datatype);
  if (len >= str_len) {
    dz_msg(DZ_LOG_ERROR,
           "string lenght (%d) is greater than str_len (%d)\n",
           len,
           str_len);
    return NULL;
  }
  // Get the dataspace of the dataset
  hid_t dataspace = H5Dget_space(dataset);
  // Allocate memory for the string buffer
  char* str_buffer = (char*)malloc(str_len + 1);
  if (!str_buffer)
    return NULL;

  // Read the dataset into the buffer and add a null terminator
  ret = H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, str_buffer);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "read hdf5 file failed");
    if (str_buffer)
      free(str_buffer);
    return NULL;
  }
  str_buffer[len] = '\0';

  // Close the datatype, dataspace, and dataset
  H5Tclose(datatype);
  H5Sclose(dataspace);
  H5Dclose(dataset);
  return str_buffer;
}

/*
 * Function: hdf5_writedata
 * ------------------------
 * Writes binary data to an HDF5 file.
 *
 * h5fd: file handle for the HDF5 file.
 * data: pointer to the binary data to be written.
 * datalen: length of the binary data in bytes.
 *
 * Returns: 0 on success, -1 on failure.
 */
ssize_t
hdf5_writedata(hid_t h5fd,
               const char* dataset_name,
               const char* data,
               size_t datalen)
{
  ssize_t ret;
  // Check for valid file id and data input
  if (h5fd <= 0 || !data || !dataset_name)
    return -1;

  // Create a scalar dataspace
  hid_t str_dataspace = H5Screate(H5S_SCALAR);

  // Create a datatype for the string data
  hid_t str_datatype = H5Tcopy(H5T_C_S1);
  H5Tset_size(str_datatype, datalen);

  // Create a dataset with the given name, datatype, and dataspace
  hid_t str_dataset = H5Dcreate(h5fd,
                                dataset_name,
                                str_datatype,
                                str_dataspace,
                                H5P_DEFAULT,
                                H5P_DEFAULT,
                                H5P_DEFAULT);

  // Write the data to the dataset
  ret =
    H5Dwrite(str_dataset, str_datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "write hdf5 file failed");
    return -1;
  }
  // Close the datatype, dataspace, and dataset
  H5Tclose(str_datatype);
  H5Sclose(str_dataspace);
  H5Dclose(str_dataset);
  // Return 0 for success
  return 0;
}
/*
 * Function: hdf5_readdata
 * -----------------------
 * Reads binary data from an HDF5 file.
 *
 * h5fd: file handle for the HDF5 file.
 * dataset_name: name of the dataset to be read.
 * buf: buffer to store the read data.
 * buflen: length of the buffer in bytes.
 *
 * Returns: number of bytes read on success, -1 on failure.
 */
ssize_t
hdf5_readdata(hid_t h5fd, const char* dataset_name, void* buf, size_t buflen)
{
  ssize_t bytes_read;
  // Check for valid file id and buffer input
  if (h5fd <= 0 || !buf)
    return -1;

  // Open the dataset with the given name
  hid_t dataset = H5Dopen(h5fd, dataset_name, H5P_DEFAULT);
  if (dataset < 0)
    return -1;

  // Get the datatype and dataspace of the dataset
  hid_t datatype = H5Dget_type(dataset);
  hid_t dataspace = H5Dget_space(dataset);

  int len = H5Tget_size(datatype);
  if (len >= buflen) {
    dz_msg(DZ_LOG_ERROR,
           "string lenght (%d) is greater than str_len (%d)\n",
           len,
           buflen);
    return -1;
  }
  // Read the data from the dataset
  bytes_read = H5Dread(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, buf);
  if (bytes_read < 0) {
    dz_msg(DZ_LOG_ERROR, "read hdf5 file failed");
    return -1;
  }
  // Close the datatype, dataspace, and dataset
  H5Tclose(datatype);
  H5Sclose(dataspace);
  H5Dclose(dataset);

  // Return the number of bytes read
  return len;
}
/*general function to compress data*/
int
compress_data(dz_alg_t method,
              char** dstdata,
              int* dstlen,
              char* srcdata,
              int srclen)
{
  switch (method) {
    case DZ_ALG_ZLIB:
      return compressdata_zlib(dstdata, dstlen, srcdata, srclen);
      break;
    case DZ_ALG_ZSTD:
      return compressdata_zstd(dstdata, dstlen, srcdata, srclen);
      break;
    default:
      return -1;
  }
}

/*general function to decompress data*/
int
decompress_data(dz_alg_t method,
                char** dstdata,
                int* dstlen,
                char* zipdata,
                int ziplen)
{
  switch (method) {
    case DZ_ALG_ZLIB:
      return decompressdata_zlib(dstdata, dstlen, zipdata, ziplen);
      break;
    case DZ_ALG_ZSTD:
      return decompressdata_zstd(dstdata, dstlen, zipdata, ziplen);
      break;
    default:
      return -1;
  }
}

int
convert2h5(const char* path, dz_alg_t method)
{
  char* h5name = NULL;
  char* tmp_h5name = NULL;
  struct stat stbuf;
  int len;
  int ret;
  int count = 0;
  hid_t h5fd;
  int srcfd;
  ssize_t nbread;
  ssize_t nbwrite;
  char buffer[H5_BUFSIZE] = { 0 };
  char version[ZIP_VERSION];
  
  if (!path) {
    return -1;
  }
  h5name = buildHiddenFile(path);
  if (!h5name)
    return -1;
  ret = stat(h5name, &stbuf);
  if (ret == 0){
      //hiddenFile already existed
      errno = EEXIST;
      ret = -1;
      goto out;
  }
  tmp_h5name = (char*)malloc(strlen(h5name) + 5);
  sprintf(tmp_h5name, "%s.tmp", h5name);
  ret = stat(tmp_h5name, &stbuf);
  if (ret == 0){
      //temporary hidden H5File already existed
      errno = EEXIST;
      ret = -1;
      goto out;
  }
  srcfd = open(path, O_RDONLY);
  if (srcfd < 0) {
    dz_msg(DZ_LOG_ERROR, "failed to open src file %s\n", path);
    ret = -1;
    goto out;
  }
  h5fd = hdf5_open(tmp_h5name, H5_WRITE);
  if (h5fd < 0) {
    dz_msg(DZ_LOG_ERROR, "failed to open hdf5 file %s\n", tmp_h5name);
    ret = -1;
    goto out;
  }
  /* write algrithom name and version*/
  switch (method) {
    case DZ_ALG_ZLIB:
      sprintf(version, "zlib %s", zlibVersion());
      ret = hdf5_writestr(h5fd, "algorithm", version);
      break;
    case DZ_ALG_ZSTD:
      sprintf(version, "zstd %s", ZSTD_versionString());
      ret = hdf5_writestr(h5fd, "algorithm", version);
      break;
    default:
      ret = -1;
      break;
  }
  if (ret < 0)
    goto out;
  /* read data from src file and write to hdf5 file */
  while (1) {
    char* dstdata = NULL;
    char dataset_name[DATASET_NAMELEN] = { 0 };
    int dstlen;
    memset(buffer, 0, H5_BUFSIZE);
    nbread = read(srcfd, buffer, H5_BUFSIZE);
    if (nbread == 0)
      break;
    if (nbread < 0) {
      dz_msg(DZ_LOG_ERROR, "failed to read src file %s\n", strerror(errno));
      ret = -1;
      goto out;
    }

    ret = compress_data(method, &dstdata, &dstlen, buffer, nbread);
    if (ret < 0) {
      dz_msg(DZ_LOG_ERROR, "failed to compress data\n");
      goto out;
    }

    sprintf(dataset_name, "block%d", count++);
    //printf("write %s len=%d srclen=%d\n", dataset_name, dstlen, nbread);
    ret = hdf5_writedata(h5fd, dataset_name, dstdata, dstlen);
    // ret = hdf5_writedata(h5fd, dataset_name, buffer, nbread);
    free(dstdata);
    dstdata = NULL;
    if (ret < 0) {
      dz_msg(DZ_LOG_ERROR, "failed to write hdf5 file");
      goto out;
    }
  }
  /*change the temperorary name to hiddenFile*/
  ret = rename(tmp_h5name, h5name);
  /* If change name failed */
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR,
           "rename from %s to %s error %s\n",
           tmp_h5name,
           h5name,
           strerror(errno));
    goto out;
  }
  //TODO: Check the file if is opened
  ret = stat(path, &stbuf);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "stat %s error %s\n", path, strerror(errno));
    goto out;
  }
  ret = truncate(path, 0);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "truncate %s error %s\n", path, strerror(errno));
    goto out;
  }
  ret = truncate(path, stbuf.st_size);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "truncate %s error %s\n", path, strerror(errno));
    goto out;
  }
out:
  if (h5name)
    free(h5name);
  if (tmp_h5name)
    free(tmp_h5name);
  if (srcfd > 0)
    close(srcfd);
  if (h5fd > 0)
    H5Fclose(h5fd);
  return ret;
}
dz_alg_t
getAlgorithm(hid_t h5fd)
{
  dz_alg_t method;
  char* algorithm = hdf5_readstr(h5fd, "algorithm", 1024);
  ssize_t len;
  if (!algorithm) {
    dz_msg(DZ_LOG_ERROR, "couldn't find compression algorithm in hdf5\n");
    return DZ_ALG_UNKOWN;
  }
  len = strlen(algorithm);
  for (int i = 0; i < len; i++) {
    if ((algorithm[i] == ' ') || (algorithm[i] == 32)) { // space
      algorithm[i] = '\0';
      len = i;
      break;
    }
  }
  method = DZ_ALG_UNKOWN;
  if (strcmp(algorithm, "zlib") == 0)
    method = DZ_ALG_ZLIB;
  if (strcmp(algorithm, "zstd") == 0)
    method = DZ_ALG_ZSTD;
  if (strcmp(algorithm, "isal") == 0)
    method = DZ_ALG_ISAL;
  free(algorithm);
  return method;
}
/*read - read from a hdf5 file descriptor*/
ssize_t
h5fd_read(hid_t h5fd, void* buf, size_t count)
{
  char* algorithm = hdf5_readstr(h5fd, "algorithm", 1024);
  ssize_t len;
  if (!algorithm) {
    dz_msg(DZ_LOG_ERROR, "couldn't find compression algorithm in hdf5\n");
    return -1;
  }
  len = strlen(algorithm);
  memcpy(buf, algorithm, count);
  free(algorithm);
  return len;
}

/*read from a hdf5 file descriptor at a given offset*/
ssize_t
h5fd_pread(hid_t h5fd, void* buf, size_t count, off_t offset)
{
  int ret;
  char* dstdata = NULL;
  int dstlen;
  off_t curoff = offset; /*current offset*/
  ssize_t nbread = 0;
  if (h5fd <= 0) {
    return -1;
  }
  dz_alg_t method = getAlgorithm(h5fd);
  if (method == DZ_ALG_UNKOWN) {
    return -1;
  }
  while (nbread < count) {
    char dataset_name[DATASET_NAMELEN] = { 0 };
    int blocknum = (int)(curoff / H5_BUFSIZE);
    char* srcp = NULL;
    char* dstp = NULL;
    char zipdata[H5_BUFSIZE] = { 0 };
    int ziplen;
    int validbytes = 0;
    // memset(zipdata, 0, H5_BUFSIZE);
    /* read corresponing data from dataset in hdf5 file */
    sprintf(dataset_name, "block%d", blocknum);
    ziplen = hdf5_readdata(h5fd, dataset_name, zipdata, H5_BUFSIZE);
    if (ziplen <= 0) {
      dz_msg(DZ_LOG_ERROR, "read dataset %s error\n", dataset_name);
      nbread = -1;
      goto out;
    }
    ret = decompress_data(method, &dstdata, &dstlen, zipdata, ziplen);
    if (ret < 0) {
      dz_msg(DZ_LOG_ERROR, "decompress dataset %s error\n", dataset_name);
      nbread = -1;
      goto out;
    }
    srcp = dstdata + curoff - blocknum * H5_BUFSIZE;
    validbytes = dstlen - curoff + blocknum * H5_BUFSIZE;
    if (validbytes == 0) {
      //nbread = 0;
      goto out;
    }
    if (validbytes > (count - nbread)) {
      validbytes = count - nbread;
    }
    //memmove(buf + nbread, srcp, validbytes);
    memmove((char *)buf + nbread, (const char *)srcp, validbytes);
    nbread += validbytes;
    curoff += validbytes;
  }
out:
  if (dstdata)
    free(dstdata);
  return nbread;
}