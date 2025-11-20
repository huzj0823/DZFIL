#ifndef _DZFILE_API_H
#define _DZFILE_API_H

#include <stdint.h>
#include <sys/uio.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

int dzfile_init(const char *data_path, const char *deflog);
int dzfile_fini();
int dzfile_creat(const char *filename, mode_t mode);
int dzfile_open(const char *filename, int flags, ...);
//int dzfile_open (const char *filename, int flags, mode_t mode);
int dzfile_close(int fd);
ssize_t dzfile_write(int fd, const void *buf, size_t count);
ssize_t dzfile_pwrite(int fd, const void *buf, size_t count, off_t offset);
ssize_t dzfile_read(int fd, void *buf, size_t count);
ssize_t dzfile_pread(int fd, void *buf, size_t count, off_t offset);
int dzfile_fstat(int fd, struct stat *buf);
int dzfile_stat(const char *filename, struct stat *buf);
int dzfile_lstat(const char *path, struct stat *buf);
off_t dzfile_lseek(int fd, off_t offset, int whence);
int dzfile_truncate(const char *path, off_t length);
int dzfile_ftruncate(int fd, off_t length);
int dzfile_fsync(int fd);
int dzfile_fdatasync(int fd);
int dzfile_statvfs(const char *path, struct statvfs *buf);
int dzfile_fstatvfs(int fd, struct statvfs *buf);
int dzfile_statfs(const char *path, struct statfs *buf);
int dzfile_fstatfs(int fd, struct statfs *buf);
int dzfile_fallocate(int fd, int mode, off_t offset, off_t len);
ssize_t dzfile_readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t dzfile_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t dzfile_preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t dzfile_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
int dzfile_lk(int fd, int32_t cmd,  struct flock *lock);
int dzfile_unlink(const char *filename);
int dzfile_mkdir(const char *pathname, mode_t mode);
DIR *dzfile_opendir(const char *name);

//void* unlink_file_thread (void *arg);

#ifdef __cplusplus
}
#endif
#endif
