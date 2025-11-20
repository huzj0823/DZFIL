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


int helpflg;
int verbose;
int errflg;

int printver()
{
    fprintf(stdout,
            "DZFILE is a embedded virtual file system which performs inline lossless compression such as lib, zstd, isa-l, and so on.\n");
    fprintf(stdout, "Version: %s-%s\n", VERSION, RELEASE);
    return 0;
}
int main(int argc, char *argv[])
{

    static struct option longopts[] = {
        {"help", no_argument, &helpflg, 1},
        {"verbose", no_argument, &verbose, 1},
        {0, 0, 0, 0}
    };
    char    *data_path = NULL;
    int c;

    while ((c = getopt_long(argc, argv, "Vvdh", longopts, NULL)) != EOF) {
        switch (c) {
        case 'h':
            helpflg = 1;
            break;
        case 'd':
            data_path = optarg;
        case 'v':
            verbose = 1;
            break;
        case 'V':
            printver();
            break;
        case '?':
            errflg++;
            break;
        default:
            break;
        }
    }
    if (errflg || helpflg) {
        printf("usage: %s -d data_path [default: /home/chyd/dzfile] -V\n", argv[0]);
        printf("\tfor example %s -V\n", argv[0]);
        return -1;
    }
    printver();
    return 0;
}


