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
#include <getopt.h>
#include "dzfile-api.h"

#define BUFLEN 524288
int helpflg;
int verbose;
int errflg;

int main(int argc, char *argv[])
{
    int     ret;
    int c;
    char    buf[BUFLEN];
    int     fd;
    int     dstfd;
    int     i;
    int     nbread = 0;
    int     nbwrite = 0;
    int     nbtotal = 0;
    struct  stat stbuf;
    char    *realpath = NULL;
    char    *data_path = NULL;
    char    *src_file = NULL;
    char    *srcp = NULL;

    static struct option longopts[] = {
        {"help", no_argument, &helpflg, 1},
        {"verbose", no_argument, &verbose, 1},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "hd:v", longopts, NULL)) != EOF) {
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
        default:
            break;
        }
    }
    if (optind >= argc) {
        errflg++;
    }
    if (errflg || helpflg) {
        printf("usage: %s -d data_path [default: /home/chyd/dzfile] filename\n",
               argv[0]);
        printf("\tfor example %s hosts\n", argv[0]);
        return -1;
    }
    srcp = argv[optind];
    if (data_path == NULL) {
        data_path = "/home/chyd/dzfile";
    }
    src_file = (char *)malloc(strlen(data_path) + strlen(srcp) + 2);
    if (!src_file) {
        printf("Failed to allocate memory for src_file %s %s\n", srcp, strerror(errno));
        return (-1);
    }
    sprintf(src_file, "%s/%s", data_path, srcp);
    printf("DEBUG: data_path=%s file=%s\n", data_path, src_file);

    ret = dzfile_init(data_path, NULL);
    if (ret < 0) {
        if(src_file) {
            free(src_file);
        }
        return -1;
    }
    fd = dzfile_open(src_file, O_RDONLY, 0644);
    if (fd <= 0) {
        printf("Failed to open file error %s %s\n", src_file, strerror(errno));
        if(src_file) {
            free(src_file);
        }
        return (-1);
    }
    if(src_file) {
        free(src_file);
    }
    ret = dzfile_fstat(fd, &stbuf);
    //for (i=0;i<1024;i++) {
    do {
        bzero(buf, BUFLEN);
        if ((nbread = dzfile_read(fd, buf, BUFLEN)) < 0) {
            printf("%s:%d read file %d error: %s\n", __FUNCTION__, __LINE__, fd, strerror(errno));
            goto out;
        }
        if (nbread == 0) {
            break;
        }
        nbtotal += nbread;
        printf("%s", buf);
    } while (nbread > 0);
    ret = dzfile_close(fd);
    if (ret < 0) {
        printf("close error: %s\n", strerror(errno));
    }
    ret = dzfile_fini();
    printf("size: %ld, read %ld\n", stbuf.st_size, nbtotal);
out:
    return ret;
}
