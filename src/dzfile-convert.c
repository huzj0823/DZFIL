#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <hdf5.h>
#include "dzfile-intern.h"
#include "dzfile-api.h"


int helpflg;
int verbose;
int errflg;



int main(int argc, char *argv[])
{

    static struct option longopts[] = {
        {"help", no_argument, &helpflg, 1},
        {"verbose", no_argument, &verbose, 1},
        {0, 0, 0, 0}
    };
    char    *data_path = "/home/chyd/dzfile";
    char    *filename = NULL;
    char    *path = NULL;
    int c;
    int ret;
    hid_t h5fd;
    struct stat stbuf;

    while ((c = getopt_long(argc, argv, "Vvd:hf:", longopts, NULL)) != EOF) {
        switch (c) {
        case 'h':
            helpflg = 1;
            break;
        case 'd':
            data_path = optarg;
        case 'v':
            verbose = 1;
            break;
        case '?':
            errflg++;
            break;
        case 'f':
            filename = optarg;
            break;
        default:
            break;
        }
    }
    if (!filename)
        errflg++;
    if (errflg || helpflg) {
        printf("usage: %s -d data_path [default: /home/chyd/dzfile] -f filename -V\n", argv[0]);
        printf("\tfor example %s -V\n", argv[0]);
        return -1;
    }
    ret = dzfile_init(data_path, NULL);
    if (ret < 0){
        printf("dzfie_init error %s:%d\n", __FUNCTION__, __LINE__);
        return -1;
    }
    path = (char *)malloc(strlen(filename)+strlen(data_path)+3);
    sprintf(path, "%s/%s", data_path, filename);
    printf("###DEBUG: path = %s\n", path);
    ret = convert2h5(path, DZ_ALG_ZSTD);
    if (ret < 0) {
        if (errno == EEXIST) {
            printf("hiddenfile is already exist\n");
        } 
        else {
            printf("conver h5 file failed\n");
            return -1;
        } 
    }
    off_t offset = 0;
    char *buffer = (char *)malloc(H5_BUFSIZE);
    int fd = dzfile_open(path, O_RDONLY);
    while (1) {
        memset(buffer, 0, H5_BUFSIZE);
        ret = dzfile_pread(fd, buffer, H5_BUFSIZE, offset);
        if (ret < 0){
            break;
        }
        //printf("%s",buffer);
        if (ret == 0) {
            break;
        }
        offset += ret;
    }
#if 0
   
    /*
    ret = convert2h5("/home/chyd/dzfile/1.json", DZ_ALG_ZSTD);
    if (ret < 0) {
        printf("conver h5 file failed\n");
    }
    */
    h5fd = hdf5_open("/home/chyd/dzfile/.1.json.DZF", H5_READ);
    if (h5fd <= 0){
        printf("open hdf5 failed %s\n", strerror(errno));
        return -1;
    }
    
    /*
    char buf[100] = {0};
    ret = h5fd_read(h5fd, buf, 100);
    printf("ret = %d\n%s\n",ret, buf);
    */
    off_t offset = 0;
    char *buffer = (char *)malloc(H5_BUFSIZE);
    while (1) {
        memset(buffer, 0, H5_BUFSIZE);
        ret = h5fd_pread(h5fd, buffer, H5_BUFSIZE, offset);
        if (ret < 0){
            break;
        }
        printf("%s",buffer);
        if (ret == 0) {
            break;
        }
        offset += ret;
    }
    if (h5fd > 0)
        hdf5_close(h5fd);
#endif
    dzfile_close(fd);
    if (path)
        free(path);
    if (buffer)
        free(buffer);
    return 0;
}


