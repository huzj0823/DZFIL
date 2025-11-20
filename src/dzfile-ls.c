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

#include "dzfile-api.h"

#define BUFLEN      524288
#define SIXMONTHS   (6*30*24*60*60)
int helpflg;
int verbose;
int errflg;
int lflag;
int iflag;
int mflag;
time_t current_time;

int listentry(const char *path, struct stat *statbuf)
{
    struct group *gr;
    char modestr[11];
    struct passwd *pw;
    static gid_t sav_gid = -1;
    static char sav_gidstr[7];
    time_t ltime;
    static uid_t sav_uid = -1;
    static char sav_uidstr[255];
    char timestr[13];
    struct tm *tm;
    char tmpbuf[21];
    if (iflag) {
        printf("%lld ", statbuf->st_ino);
    }
    if (lflag) {
        if (statbuf->st_mode & S_IFDIR) {
            modestr[0] = 'd';
        } else {
            modestr[0] = '-';
        }
        modestr[1] = (statbuf->st_mode & S_IRUSR) ? 'r' : '-';
        modestr[2] = (statbuf->st_mode & S_IWUSR) ? 'w' : '-';
        if (statbuf->st_mode & S_IXUSR)
            if (statbuf->st_mode & S_ISUID) {
                modestr[3] = 's';
            } else {
                modestr[3] = 'x';
            } else {
            modestr[3] = '-';
        }
        modestr[4] = (statbuf->st_mode & S_IRGRP) ? 'r' : '-';
        modestr[5] = (statbuf->st_mode & S_IWGRP) ? 'w' : '-';
        if (statbuf->st_mode & S_IXGRP)
            if (statbuf->st_mode & S_ISGID) {
                modestr[6] = 's';
            } else {
                modestr[6] = 'x';
            } else {
            modestr[6] = '-';
        }
        modestr[7] = (statbuf->st_mode & S_IROTH) ? 'r' : '-';
        modestr[8] = (statbuf->st_mode & S_IWOTH) ? 'w' : '-';
        if (statbuf->st_mode & S_IXOTH)
            if (statbuf->st_mode & S_ISVTX) {
                modestr[9] = 't';
            } else {
                modestr[9] = 'x';
            } else {
            modestr[9] = '-';
        }
        modestr[10] = '\0';
        if (statbuf->st_uid != sav_uid) {
            sav_uid = statbuf->st_uid;
            if (pw = getpwuid(sav_uid)) {
                strcpy(sav_uidstr, pw->pw_name);
            } else {
                sprintf(sav_uidstr, "%-8u", sav_uid);
            }
        }
        if (statbuf->st_gid != sav_gid) {
            sav_gid = statbuf->st_gid;
            if (gr = getgrgid(sav_gid)) {
                strncpy(sav_gidstr, gr->gr_name, sizeof(sav_gidstr) - 1);
                sav_gidstr[sizeof(sav_gidstr) - 1] = '\0';
            } else {
                sprintf(sav_gidstr, "%-6u", sav_gid);
            }
        }
        ltime = statbuf->st_mtime;
        tm = localtime(&ltime);
        if (ltime < current_time - SIXMONTHS ||
                ltime > current_time + 60) {
            strftime(timestr, 13, "%b %d  %Y", tm);
        } else {
            strftime(timestr, 13, "%b %d %H:%M", tm);
        }
        printf("%s %3d %-8.8s %-6.6s%20ld\t%s ",
               modestr, statbuf->st_nlink, sav_uidstr, sav_gidstr,
               statbuf->st_size, timestr);
    }
    printf("%s", path);
    printf("\n");
    return (0);
}
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
    char    *meta_path = NULL;
    char    *data_path = NULL;
    char    *src_file = NULL;
    char    *srcp = NULL;
    DIR *dirp = NULL;
    struct  dirent* dentp = NULL;

    static struct option longopts[] = {
        {"help", no_argument, &helpflg, 1},
        {"verbose", no_argument, &verbose, 1},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "hd:vlim", longopts, NULL)) != EOF) {
        switch (c) {
        case 'h':
            helpflg = 1;
            break;
        case 'd':
            data_path = optarg;
        case 'v':
            verbose = 1;
            break;
        case 'l':
            lflag = 1;
            break;
        case 'i':
            iflag = 1;
            break;
        case 'm':
            mflag = 1;
            break;
        case '?':
            errflg++;
            break;
        default:
            break;
        }
    }
    if (errflg || helpflg) {
        printf("usage: %s -m meta_path [default: /data8/dzfile] -d data_path [default: /home/chyd/dzfile] [OPTION] [filename]\n",
               argv[0]);
        printf("Option can be -l, -i, -d\n");
        printf("\tfor example %s -l hosts\n", argv[0]);
        return -1;
    }
    (void) time(&current_time);
    if (optind >= argc) {
        srcp = "";
    } else {
        srcp = argv[optind];
    }
    if (data_path == NULL) {
        data_path = "/home/chyd/dzfile";
    }
    src_file = (char *)malloc(strlen(data_path) + strlen(srcp) + 2);
    if (!src_file) {
        printf("Failed to allocate memory for src_file %s %s\n", srcp, strerror(errno));
        return (-1);
    }
    sprintf(src_file, "%s/%s", data_path, srcp);
    if (verbose) {
        printf("DEBUG: data_path=%s file=%s\n", data_path, src_file);
    }
    ret = dzfile_init(data_path, NULL);
    if (ret < 0) {
        if(src_file) {
            free(src_file);
        }
        return -1;
    }
    ret = dzfile_stat(src_file, &stbuf);
    if (ret < 0) {
        printf("get stat for  %s failed %s\n", src_file, strerror(errno));
        goto out;
    }
    if (!(stbuf.st_mode & S_IFDIR) || mflag) {
        listentry(src_file, &stbuf);
        goto out;
    }

    dirp = dzfile_opendir(src_file);
    if (!dirp) {
        printf("open dir %s failed %s\n", src_file, strerror(errno));
        goto out;
    }
    while ((dentp = dzfile_readdir(dirp)) != NULL) {
        char entname[1024];
        if ((strcmp(dentp->d_name, ".") == 0) || (strcmp(dentp->d_name, "..") == 0)
                || (strcmp(dentp->d_name, ".glusterfs") == 0)) {
            continue;
        }
        if (!lflag && !iflag) {
            printf("%s ", dentp->d_name);
            continue;
        }
        sprintf(entname, "%s/%s", src_file, dentp->d_name);
        ret = dzfile_stat(entname, &stbuf);
        if (ret < 0) {
            printf("get stat for  %s failed %s\n", entname, strerror(errno));
            continue;
        }
        listentry(dentp->d_name, &stbuf);
    }
    if (!lflag && !iflag) {
        printf("\n");
    }
    //ret = dzfile_stat(, &stbuf);
    //for (i=0;i<1024;i++) {
out:
    if (dirp) {
        closedir(dirp);
    }
    if(src_file) {
        free(src_file);
    }
    dzfile_fini();
    return ret;
}


