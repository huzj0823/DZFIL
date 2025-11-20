#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>

int main() {
    const char* filename = "./f1";
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("Error opening file: %s\n", strerror(errno));
        return 1;
    }

    int rc = flock(fd, LOCK_EX | LOCK_NB);
    if ((rc == -1) && (errno == EWOULDBLOCK))  {
        printf("File is already open, this fd=%d\n", fd);
        close(fd);
        return 1;
    }

    printf("File is not open\n");
    printf("This fd=%d\n", fd);
    flock(fd, LOCK_UN);

    close(fd);
    return 0;
}
