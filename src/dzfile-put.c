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
#include <sys/time.h>
#include <pthread.h>
#include "dzfile-api.h"

#define BUFLEN 524288
uint64_t total_bytes = 0;
uint64_t last_bytes = 0;
struct timeval last_time;
struct timeval first, finish;
pthread_mutex_t size_lock;

int helpflg;
int verbose;
int errflg;
int stopflg;

float computelapse(struct timeval ftime, struct timeval etime)
{
    struct timeval lapsed;
    long start, end;
    float tsend;
    start = ftime.tv_sec * 1000000 + ftime.tv_usec;
    end = etime.tv_sec * 1000000 + etime.tv_usec;
    tsend = (float)(end - start) / 1000000;
    // fprintf(stdout,"tsend is \t%.3fs\n",tsend);
    return tsend;
}

void *perf_mon(void *arg)
{
    struct timezone tzp;
    struct timeval curtime;
    int ret = 0;
    float tsend;
    float atsend;
    uint64_t transferred;
    int i;
    while (stopflg == 0)
    {
        char tmpbuf[21];
        float avg;
        float inst;
        if (total_bytes == 0)
        {
            continue;
        }
        gettimeofday(&curtime, &tzp);
        atsend = computelapse(first, curtime);
        tsend = computelapse(last_time, curtime);
        transferred = total_bytes - last_bytes;
        avg = (float)total_bytes / (atsend * 1024 * 1024);
        inst = (float)transferred / (tsend * 1024 * 1024);
        fprintf(stdout, "\r %llu bytes", total_bytes);
        fprintf(stdout, " %9.2lf MB/sec avg", avg);
        fprintf(stdout, " %9.2lf MB/sec inst", inst);
        fflush(stdout);
        last_bytes = total_bytes;
        last_time = curtime;
        for (i = 0; i < 100; i++)
        {
            if (!stopflg)
            {
                usleep(10000);
            }
        }
    }
    pthread_exit((void *)&ret);
}

int dzfile_lockw(int fd, off_t offset)
{
    struct flock lock;
    int32_t cmd;
    int ret;

    while (1)
    {
        cmd = F_GETLK;
        lock.l_type = F_WRLCK;
        lock.l_start = offset;
        lock.l_whence = SEEK_SET;
        lock.l_len = BUFLEN;
        lock.l_pid = getpid();
        ret = dzfile_lk(fd, cmd, &lock);
        if (ret < 0)
        {
            printf("%d Return value of fcntl:%d %s\n", __LINE__, ret, strerror(errno));
            return (-1);
        }
        if (lock.l_type == F_UNLCK)
        {
            // printf("write lock can be setted\n");
            break;
        }
        printf("DEBUG: Already had lock (%d) by pid(%d), wait...\n", lock.l_type, lock.l_pid);
        sleep(1);
    }
    cmd = F_SETLK;
    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lock.l_whence = SEEK_SET;
    lock.l_len = BUFLEN;
    lock.l_pid = getpid();
    ret = dzfile_lk(fd, cmd, &lock);
    if (ret < 0)
    {
        printf("DEBUG: %d Return value of dzfile_lk:%d %s\n", __LINE__, ret, strerror(errno));
        return (-1);
    }
    if (lock.l_type == F_WRLCK)
    {
        // printf("DEBUG: write lock is set successfully\n");
    }
    else
    {
        printf("DEBUG: set write lock failed\n");
        return -1;
    }
    return 0;
}

int dzfile_put(const char *data_path, const char *src_file, const char *dst_file)
{
    int fd = -1;
    int dstfd = -1;
    struct stat stbuf;
    int ret = -1;
    char buf[BUFLEN];
    int nbread;
    int nbwrite;
    if (!data_path || !src_file || !dst_file)
    {
        errno = EINVAL;
        return -1;
    }
    fd = open(src_file, O_RDONLY, 0644);
    if (fd <= 0)
    {
        printf("%s:%d:%s Failed to open file %s, %s\n",
               __FILE__, __LINE__, __FUNCTION__, src_file, strerror(errno));
        return -1;
    }
    ret = dzfile_init(data_path, NULL);
    if (ret < 0)
    {
        return -1;
    }
    ret = fstat(fd, &stbuf);
    dstfd = dzfile_open(dst_file, O_WRONLY | O_CREAT, 0644);
    if (dstfd < 0)
    {
        printf("%s:%d:%s Failed to open file %s, %s\n",
               __FILE__, __LINE__, __FUNCTION__, dst_file, strerror(errno));
        goto out;
    }
    do
    {
        bzero(buf, BUFLEN);
        if ((nbread = read(fd, buf, BUFLEN)) < 0)
        {
            printf("%s:%d read file %d error: %s\n", __FUNCTION__, __LINE__, fd, strerror(errno));
            stopflg = 1;
            goto out;
        }
        if (nbread == 0)
        {
            break;
        }
        if ((ret = dzfile_lockw(dstfd, total_bytes)) < 0)
        {
            printf("%s:%d Failed to set lock on fd %d error: %s\n",
                   __FUNCTION__, __LINE__, fd, strerror(errno));
            goto out;
        }
        if ((ret = dzfile_write(dstfd, buf, nbread)) < 0)
        {
            printf("%s:%d Failed to write file %d error: %s\n",
                   __FUNCTION__, __LINE__, fd, strerror(errno));
            goto out;
        }
        total_bytes += nbread;
    } while (nbread > 0);
out:
    if (dstfd > 0)
    {
        ret = dzfile_close(dstfd);
        if (ret < 0)
        {
            printf("close error: %s\n", strerror(errno));
        }
    }
    if (fd > 0)
    {
        close(fd);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int ret;
    int c;
    int i;
    struct stat stbuf, stbuf2;
    char *realpath = NULL;
    char *data_path = NULL;
    char *src_file = NULL;
    char *dst_file = NULL;
    char *dstp = NULL;
    struct timezone tzp;
    float tsend;
    pthread_t pm_thread = 0;

    static struct option longopts[] = {
        {"help", no_argument, &helpflg, 1},
        {"verbose", no_argument, &verbose, 1},
        {0, 0, 0, 0}};

    while ((c = getopt_long(argc, argv, "hd:v", longopts, NULL)) != EOF)
    {
        switch (c)
        {
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
    if (optind + 1 >= argc)
    {
        errflg++;
    }
    if (errflg || helpflg)
    {
        printf("%s: Put a file from dzfile storage\n", argv[0]);
        printf("usage: %s %s %s\n", argv[0],
               "-d data_path [default: /home/chyd/dzfile]",
               "local_filename dzfile_filename");
        printf("\tfor example %s /etc/hosts hosts\n", argv[0]);
        return -1;
    }
    /*initial*/
    gettimeofday(&first, &tzp);
    if (verbose)
    {
        last_time = first;
        pthread_mutex_init(&size_lock, NULL);
        if (pthread_create(&pm_thread, NULL, perf_mon, NULL) < 0)
        {
            printf("%s:%s create thread failed\n", __FILE__, __LINE__, strerror(errno));
            return -1;
        }
    }
    src_file = argv[optind];
    dstp = argv[optind + 1];
    if (data_path == NULL)
    {
        data_path = "/home/chyd/dzfile";
    }
    dst_file = (char *)calloc(1, strlen(data_path) + strlen(dstp) + 2);
    if (!dst_file)
    {
        printf("%s:%d:%s Failed to allocate memory for dst_file %s %s\n",
               __FILE__, __LINE__, __FUNCTION__, dstp, strerror(errno));
        return (-1);
    }
    sprintf(dst_file, "%s/%s", data_path, dstp);
    printf("DEBUG: data_path=%s src=%s dst=%s\n",
           data_path, src_file, dst_file);

    ret = dzfile_put(data_path, src_file, dst_file);
    if (ret < 0)
    {
        goto out;
    }
    stopflg = 1;
    gettimeofday(&finish, &tzp);
    tsend = computelapse(first, finish);
    if (verbose)
    {
        printf("\n");
        pthread_join(pm_thread, NULL);
    }
    ret = stat(src_file, &stbuf);
    ret = dzfile_stat(dst_file, &stbuf2);
    if (ret == 0)
    {
        fprintf(stdout, "src_size=%lld dst_size=%lld and %lld bytes transferred in %.2f seconds (avag. %.2f MB/s)\n",
                stbuf.st_size, stbuf2.st_size, total_bytes, tsend, (float)total_bytes / (tsend * 1024 * 1024));
    }
out:
    (void)dzfile_fini();
    stopflg = 1;
    if (dst_file)
    {
        free(dst_file);
    }
    return ret;
}
