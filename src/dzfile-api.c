#include "dzfile-api.h"
#include "dz-mq.h"
#include "dzfile-intern.h"
#include "dzfile.h"
#include "sqlite-ifce.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

#define _GNU_SOURCE

static struct dzfile_priv* priv = NULL;

struct dzfile_priv*
get_priv()
{
  if (!priv) {
    return NULL;
  }
  return priv;
}

int
dzfile_init(const char* data_path, const char* deflog)
{
  char* logfile = NULL;
  int logfd = -1;
  int ret = -1;
  struct stat stbuf;
  if (!data_path)
    return -1;

  if (!deflog) {
    logfile = strdup("/var/log/dzfile/dzfile.log");
  } else {
    logfile = strdup(deflog);
  }
  logfd = dz_openlog(logfile);
  if (logfd < 0) {
    fprintf(stderr, "open log file %s error %s\n", logfile, strerror(errno));
    free(logfile);
    return -1;
  }
  priv = (struct dzfile_priv*)calloc(1, sizeof(*priv));
  if (!priv) {
    fprintf(stderr,
            "[%s:%d] calloc priv error %s\n",
            __FILE__,
            __FUNCTION__,
            strerror(errno));
    return -1;
  }
  priv->logfd = logfd;
  priv->logfile = strdup(logfile);
  priv->loglevel = DZ_LOG_DEBUG;
  ENTRY();
  ret = stat(data_path, &stbuf);
  if ((ret != 0) || !S_ISDIR(stbuf.st_mode)) {
    dz_msg(DZ_LOG_ERROR, "Directory '%s' doesn't exist, exiting.", data_path);
    goto out;
  }
  priv->data_path = strdup(data_path);
  priv->path_max = pathconf(priv->data_path, _PC_PATH_MAX);
  if (priv->path_max != -1 &&
      _XOPEN_PATH_MAX + strlen(data_path) > priv->path_max) {
    ret = chdir(priv->data_path);
    if (ret) {
      dz_msg(DZ_LOG_ERROR, "%s\n", strerror(errno));
      goto out;
    }
  }
  if (logfile) {
    free(logfile);
  }
  /* open zeromq */
  ret = dz_openmq(5555);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "failed to open zero mq\n");
    goto out;
  }
  /* open database */
  ret = dz_opendb();
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "failed to open database\n");
    goto out;
  }
  LOCK_INIT(&priv->dblock); // db lock
  hashmap_init();
  dz_msg(DZ_LOG_DEBUG, "dzfile init successfully\n");
  return 0;
out:
  END();
  return ret;
}
int
dzfile_fini()
{
  if (priv->data_path != NULL) {
    free(priv->data_path);
  }
  if (priv->logfile != NULL) {
    free(priv->logfile);
  }
  LOCK_DESTROY(&priv->dblock);
  if (priv->db)
    dz_closedb();
  if (priv) {
    free(priv);
  }
  return 0;
}

/*creat and open logfie*/
int
dz_openlog(const char* logfile)
{
  int fd;
  int ret;

  if (!logfile) {
    errno = EINVAL;
    return -1;
  }
  fd = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0644);
  if ((fd < 0) && (errno == ENOENT)) {
    ret = make_parent_dir(logfile);
    if (ret < 0) {
      return -1;
    }
    fd = open(logfile, O_WRONLY | O_CREAT, 0644);
  }
  return fd;
}

int
dzfile_creat(const char* filename, mode_t mode)
{
  int isexist;
  char* bname = strrchr(filename, '/') + 1; // base name of filename
  if ((bname[0] == '.') && (strcmp(bname + strlen(bname) - 4, ".DZF") == 0)) {
    dz_msg(DZ_LOG_ERROR, "filename '%s' is illegal like  .XXX.DZF", bname);
    errno = EACCES;
    return -1;
  }
  if ((match_sysdir(filename) == 1) || (match_hiddenfile(filename) == 1)) {
    errno = EACCES;
    return -1;
  }
  isexist = findhiddenFile(filename);
  switch (isexist) {
    case -1:
      return -1;
      break;
    case 0:
      dz_msg(DZ_LOG_DEBUG, "'%s' is created\n", filename);
      fmt_entry_insert(filename); // Tell the db that one new file is created
      dz_notifymq(filename);
      return creat(filename, mode);
      break;
    case 1:
      return EEXIST;
      break;
    case 2:
      return EISDIR;
      break;
  }
}
// int dzfile_open (const char *filename, int flags, mode_t mode)
int
dzfile_open(const char* filename, int flags, ...)
{
  mode_t mode;
  va_list args;
  char* bname = NULL;
  int ret, ret2;
  int isexist = 1;
  char* hiddenFile = NULL;
  int fd;
  hid_t h5fd;
  dzfd_t* dzfd = NULL;
  struct stat stbuf;

  if (!filename)
    return -1;
  ENTRY();
  if (match_sysdir(filename) == 1) {
    return -1;
  }
  va_start(args, flags);
  mode = va_arg(args, int);
  va_end(args);
  dz_msg(DZ_LOG_DEBUG, "Open entry file : %s, flags:0x%x \n", filename, flags);
  bname = strrchr(filename, '/') + 1; // base name of filename
  if ((bname[0] == '.') && (strcmp(bname + strlen(bname) - 4, ".DZF") == 0)) {
    dz_msg(DZ_LOG_ERROR, "filename '%s' is illegal like  .XXX.DZF\n", bname);
    errno = EACCES;
    return -1;
  }

  // The file exists
  isexist = findhiddenFile(filename);
  if (isexist == 1) { // The hidden file is already exists
    int access_mode = flags & O_ACCMODE;
    // int is_read_only = (access_mode == O_RDONLY || access_mode == O_APPEND);
    if (access_mode != O_RDONLY) {
      dz_msg(DZ_LOG_ERROR, "only support O_RDONLY or O_APPEND on %s", filename);
      errno = EACCES;
      return -1;
    }
    // TODO: Open hidden file here
    hiddenFile = buildHiddenFile(filename);
    if (hiddenFile == NULL) {
      dz_msg(DZ_LOG_ERROR, "build hiddenFile for %s error\n", filename);
      return -1;
    }
    dz_msg(DZ_LOG_DEBUG, "read hidden file '%s'", hiddenFile);
    h5fd = hdf5_open(hiddenFile, H5_READ);
    if (h5fd <= 0) {
      dz_msg(DZ_LOG_ERROR, "open hdf5 file %s error\n", hiddenFile);
      free(hiddenFile);
      return -1;
    }
    dzfd = (dzfd_t*)malloc(sizeof(dzfd_t));
    if (dzfd == NULL) {
      dz_msg(DZ_LOG_ERROR, "malloc error\n");
      free(hiddenFile);
      return -1;
    }
    free(hiddenFile);
    fd = open(filename, flags, mode);
    dzfd->fd = fd;
    dzfd->h5fd = h5fd;
    dzfd->filename = hiddenFile;
    add_dzfd(fd, dzfd);
    return fd;
  } // end of isexist==1
  if (isexist < 0) {
    dz_msg(DZ_LOG_ERROR,
           "%s exists with access error %s",
           filename,
           strerror(errno));
    return -1;
  }
  if (isexist == 2) {
    errno = EISDIR;
    return -1;
  }

  ret = lstat(filename, &stbuf);
  if (ret < 0 && errno == ENOENT) {
    dz_msg(DZ_LOG_DEBUG, "'%s' is created\n", filename);
    fmt_entry_insert(filename); // Tell the db that one new file is created
  }
  dz_notifymq(filename);
  return open(filename, flags, mode);
}

int
dzfile_close(int fd)
{
  dzfd_t* dzfd = find_dzfd(fd);
  if (dzfd) {
    hdf5_close(dzfd->h5fd);
    remove_dzfd(dzfd->fd);
    free(dzfd);
    dzfd = NULL;
  }
  return close(fd);
}
ssize_t
dzfile_write(int fd, const void* buf, size_t count)
{
  ssize_t ret = write(fd, buf, count);
  if (ret > 0) {
    add_writtenbytes(ret);
  }
  return ret;
}
ssize_t
dzfile_pwrite(int fd, const void* buf, size_t count, off_t offset)
{
  ssize_t ret = pwrite(fd, buf, count, offset);
  if (ret > 0) {
    add_writtenbytes(ret);
  }
  return ret;
}
ssize_t
dzfile_read(int fd, void* buf, size_t count)
{
  return read(fd, buf, count);
}
ssize_t
dzfile_pread(int fd, void* buf, size_t count, off_t offset)
{
  dzfd_t* dzfd = find_dzfd(fd);
  if (dzfd) {
    return h5fd_pread(dzfd->h5fd, buf, count, offset);
  }
  return pread(fd, buf, count, offset);
}
int
dzfile_fstat(int fd, struct stat* buf)
{
  return fstat(fd, buf);
}
int
dzfile_stat(const char* filename, struct stat* buf)
{
  return stat(filename, buf);
}
int
dzfile_lstat(const char* path, struct stat* buf)
{
  return lstat(path, buf);
}
off_t
dzfile_lseek(int fd, off_t offset, int whence)
{
  return lseek(fd, offset, whence);
}

int
dzfile_truncate(const char* path, off_t length)
{
  int isexist = findhiddenFile(path);
  if (isexist == 1) { // It is a existed processed file
    errno = ENOTSUP;
    return -1;
  }
  return truncate(path, length);
}
int
dzfile_ftruncate(int fd, off_t length)
{
  return ftruncate(fd, length);
}
int
dzfile_fsync(int fd)
{
  return fsync(fd);
}
int
dzfile_fdatasync(int fd)
{
  return fdatasync(fd);
}
int
dzfile_statvfs(const char* path, struct statvfs* buf)
{
  return statvfs(path, buf);
}
int
dzfile_fstatvfs(int fd, struct statvfs* buf)
{
  return fstatvfs(fd, buf);
}
int
dzfile_statfs(const char* path, struct statfs* buf)
{
  return statfs(path, buf);
}
int
dzfile_fstatfs(int fd, struct statfs* buf)
{
  return fstatfs(fd, buf);
}
int
dzfile_fallocate(int fd, int mode, off_t offset, off_t len)
{
  return posix_fallocate(fd, offset, len);
}
ssize_t
dzfile_readv(int fd, const struct iovec* iov, int iovcnt)
{
  int i;
  ssize_t count = 0;
  ssize_t nbread = 0;
  ENTRY();
  for (i = 0; i < iovcnt; i++) {
    nbread = dzfile_read(fd, iov[i].iov_base, iov[i].iov_len);
    if (nbread < 0) {
      return nbread;
    }
    if (nbread == 0) {
      break;
    }
    count += nbread;
  }
  END();
  return count;
}
ssize_t
dzfile_writev(int fd, const struct iovec* iov, int iovcnt)
{
  int i;
  ssize_t count = 0;
  ssize_t nbwrite = 0;
  ENTRY();
  for (i = 0; i < iovcnt; i++) {
    nbwrite = dzfile_write(fd, iov[i].iov_base, iov[i].iov_len);
    if (nbwrite < 0) {
      return -1;
    }
    if (nbwrite == 0) {
      break;
    }
    count += nbwrite;
  }
  if (count > 0) {
    add_writtenbytes(count);
  }
  END();
  return count;
}

ssize_t
dzfile_preadv(int fd, const struct iovec* iov, int iovcnt, off_t offset)
{
  int i;
  ssize_t count = 0;
  ssize_t nbread = 0;

  ENTRY();

  for (i = 0; i < iovcnt; i++) {
    nbread = dzfile_pread(fd, iov[i].iov_base, iov[i].iov_len, offset);
    if (nbread < 0) {
      return nbread;
    }
    if (nbread == 0) {
      break;
    }
    count += nbread;
    offset += nbread;
  }

  END();

  return count;
}
ssize_t
dzfile_pwritev(int fd, const struct iovec* iov, int iovcnt, off_t offset)
{
  int i;
  ssize_t count = 0;
  ssize_t nbwrite = 0;

  ENTRY();
  for (i = 0; i < iovcnt; i++) {
    nbwrite = dzfile_pwrite(fd, iov[i].iov_base, iov[i].iov_len, offset);
    if (nbwrite < 0) {
      return -1;
    }
    if (nbwrite == 0) {
      break;
    }
    count += nbwrite;
    offset += nbwrite;
  }
  if (count > 0) {
    add_writtenbytes(count);
  }
  END();
  return count;
}
int
dzfile_lk(int fd, int32_t cmd, struct flock* lock)
{
  int ret = 0;
  ret = fcntl(fd, cmd, lock);
  if (ret < 0) {
    dz_msg(DZ_LOG_DEBUG,
           "lock file (fd=%d cmd=%d type=%d whence=%d start=%ld len=%ld "
           "pid=%ld)failed %s\n",
           fd,
           cmd,
           lock->l_type,
           lock->l_whence,
           lock->l_start,
           lock->l_len,
           lock->l_pid,
           strerror(errno));
  }
  return ret;
}
int
dzfile_unlink(const char* filename)
{
  struct stat stbuf;
  int rc;
  if (match_sysdir(filename) == 1) {
    return -1;
  }
  dz_notifymq(filename);
  rc = findhiddenFile(filename);
  if (rc == 1) {
    char* hiddenFile = buildHiddenFile(filename);
    int ret = stat(hiddenFile, &stbuf);
    if (ret < 0)
      return -1;
    if (unlink(hiddenFile) < 0) {
      dz_msg(DZ_LOG_ERROR,
             "delete file '%s' failed %s\n",
             hiddenFile,
             strerror(errno));
      free(hiddenFile);
      return -1;
    }
    sub_diskbytes(stbuf.st_size);
  }
  rc = unlink(filename);
  if (rc < 0)
    return rc;
  rc = stat(filename, &stbuf);
  if (rc < 0)
    return -1;
  sub_writtenbytes(stbuf.st_size);
  return 0;
}
int
dzfile_mkdir(const char* pathname, mode_t mode)
{
  if (match_sysdir(pathname) == 1) {
    return -1;
  }
  return mkdir(pathname, mode);
}
DIR*
dzfile_opendir(const char* pathname)
{
  if ((match_sysdir(pathname) == 1) || (match_hiddenfile(pathname) == 1)) {
    errno = EACCES;
    return NULL;
  }
  return opendir(pathname);
}
struct dirent *dzfile_readdir(DIR *dirp)
{
  struct dirent *dent;
  dent = readdir(dirp);
  if (dent == NULL)
    return NULL;
  if ((match_hiddenfile(dent->d_name) == 1) || (strcmp(dent->d_name, ".dzs") == 0)){
    return dzfile_readdir(dirp);
  }
  return dent;
}

int
dzfile_dstat(const char* path, struct dzfile_dinfo* dbuf)
{
  struct space_entry spentry;
  int ret = -1;
  int fsflag = 0;
  struct statfs fsbuf;
  struct stat stbuf;

  VALIDATE_PRIV(out);
  VALIDATE_OR_GOTO(path, out);
  VALIDATE_OR_GOTO(dbuf, out);

  ENTRY();

  ret = statfs(path, &fsbuf);
  if (ret < 0) {
    return -1;
  }
  dbuf->f_files = fsbuf.f_files;
  dbuf->capacity = fsbuf.f_bsize * fsbuf.f_blocks;
  dbuf->usedspace = fsbuf.f_bsize * (fsbuf.f_blocks - fsbuf.f_bfree);
  dbuf->filenum = 0;
  dbuf->totalsize = 0;
  dbuf->disksize = 0;
  dbuf->comratio = 1;
  ret = get_space_entry(&spentry);
  if (ret < 0)
    return -1;
  dbuf->filenum = spentry.filenum;
  dbuf->totalsize = spentry.writtenbytes;
  dbuf->disksize = spentry.diskbytes;

  ret = stat(path, &stbuf);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "stat file '%s' failed %s\n", path, strerror(errno));
    return -1;
  }
  if (S_ISDIR(stbuf.st_mode)) {
    return 0;
  }
  dbuf->filenum = 1;
  dbuf->totalsize = stbuf.st_size;
  dbuf->disksize = stbuf.st_size;
  if (findhiddenFile(path) == 1) { // The hidden file exists
    char* hiddenFile = buildHiddenFile(path);
    if (!hiddenFile)
      return -1;
    ret = stat(hiddenFile, &stbuf);
    free(hiddenFile);
    if (ret < 0) {
      return -1;
    }
    dbuf->disksize = stbuf.st_size;
    dbuf->comratio = dbuf->totalsize / dbuf->disksize;
  }
out:
  return ret;
}
