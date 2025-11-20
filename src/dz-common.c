#include "common.h"
#include "dzfile.h"
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int
make_parent_dir(const char* path)
{
  int ret = -1;
  int rc;
  char* dirc = NULL;
  char* pardir;
  struct stat stbuf;

  dirc = strdup(path);
  VALIDATE_OR_GOTO(dirc, out);
  pardir = dirname(dirc);
  ret = lstat(pardir, &stbuf);
  if (ret == 0) {
    goto out;
  }
  if (ret < 0 && errno != ENOENT) {
    FAILRET();
  }
  ret = mkdir(pardir, 0644);
  /*A component of the path prefix specified by path does not name an existing
   * directory or path is an empty string */
  if (ret < 0 && errno == ENOENT) {
    rc = make_parent_dir(pardir);
    if (rc == 0) {
      ret = mkdir(pardir, 0644);
    }
    goto out;
  }
  if (ret < 0 && errno == EEXIST) {
    ret = 0;
    goto out;
  }
  if (ret < 0) {
    FAILRET();
  }
out:
  if (dirc) {
    free(dirc);
  }
  return ret;
}

int
intstr_length(uint64_t i)
{
  char tmpstr[21];

  sprintf(tmpstr, "%llu", i);
  return (strlen(tmpstr));
}

/* Check if we should be logging
 * Return value: _dz_false : Print the log
 *              _dz_true : Do not Print the log
 */
static bool
skip_logging(dz_loglevel_t level)
{
  int ret = false;
  struct dzfile_priv* priv = get_priv();
  dz_loglevel_t existing_level = DZ_LOG_NONE;

  VALIDATE_PRIV(out);
  if (level == DZ_LOG_NONE) {
    ret = true;
    goto out;
  }
  existing_level = priv->loglevel;
  if (level > existing_level) {
    ret = true;
    goto out;
  }
out:
  return ret;
}

int
_dz_msg(const char* domain,
        const char* file,
        const char* function,
        int32_t line,
        dz_loglevel_t level,
        int errnum,
        int trace,
        uint64_t msgid,
        const char* fmt,
        ...)
{
  int ret = -1;
  char callstr[DZ_LOG_BACKTRACE_SIZE] = {
    0,
  };
  int passcallstr = 0;
  int log_inited = 0;
  va_list args;
  char prtbuf[DZ_LOGBUFSZ];
  int save_errno;
  struct timeval tv = {
    0,
  };
  struct tm* tm = NULL;
  int fd_log;
  int nbwrite;
  char levchar;
  struct dzfile_priv* priv = get_priv();

  VALIDATE_PRIV(out);
  /* in args check */
  if (!domain || !file || !function || !fmt) {
    fprintf(stderr,
            "logging: %s:%s():%d: invalid argument\n",
            __FILE__,
            __PRETTY_FUNCTION__,
            __LINE__);
    return -1;
  }
  /* check if we should be logging */
  if (skip_logging(level)) {
    return 0;
  }
  if (trace) {
    ret =
      _dz_msg_backtrace(DZ_LOG_BACKTRACE_DEPTH, callstr, DZ_LOG_BACKTRACE_SIZE);
    if (ret >= 0) {
      passcallstr = 1;
    } else {
      ret = 0;
    }
    goto out;
  }
  fd_log = priv->logfd;
  levchar = _dz_log_char(level);
  /* form the message */
  save_errno = errno;
  va_start(args, fmt);
  ret = gettimeofday(&tv, NULL);
  if (ret < 0) {
    goto out;
  }
  tm = localtime(&tv.tv_sec);
  snprintf(prtbuf,
           sizeof(prtbuf),
           "[%4d-%02d-%02d %02d:%02d:%02d.%06d] %c [%s:%d:%s] %d-",
           tm->tm_year + 1900,
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec,
           tv.tv_usec,
           levchar,
           file,
           line,
           function,
           errnum);
  vsnprintf(
    prtbuf + strlen(prtbuf), (sizeof(prtbuf) - strlen(prtbuf)), fmt, args);
  va_end(args);
  ret = write(fd_log, prtbuf, strlen(prtbuf));
  errno = save_errno;
out:
  return ret;
}

static int
_dz_msg_backtrace(int stacksize, char* callstr, size_t strsize)
{
  int ret = -1;
  int i = 0;
  int size = 0;
  int savstrsize = strsize;
  void* array[200];
  char** callingfn = NULL;

  /* We chop off last 2 anyway, so if request is less than tolerance
   *          * nothing to do */
  if (stacksize < 3) {
    goto out;
  }
  size = backtrace(array, ((stacksize <= 200) ? stacksize : 200));
  if ((size - 3) < 0) {
    goto out;
  }
  if (size) {
    callingfn = backtrace_symbols(&array[2], size - 2);
  }
  if (!callingfn) {
    goto out;
  }
  ret = snprintf(callstr, strsize, "(");
  PRINT_SIZE_CHECK(ret, out, strsize);
  for ((i = size - 3); i >= 0; i--) {
    ret =
      snprintf(callstr + savstrsize - strsize, strsize, "-->%s ", callingfn[i]);
    PRINT_SIZE_CHECK(ret, out, strsize);
  }
  ret = snprintf(callstr + savstrsize - strsize, strsize, ")");
  PRINT_SIZE_CHECK(ret, out, strsize);
out:
  free(callingfn);
  return ret;
}

static char
_dz_log_char(int loglevel)
{
  char d = 'N';

  switch (loglevel) {
    case DZ_LOG_NONE:
      d = 'N';
      break;
    case DZ_LOG_EMERG:
      d = 'E';
      break;
    case DZ_LOG_ALERT:
      d = 'A';
      break;
    case DZ_LOG_CRITICAL:
      d = 'C';
      break;
    case DZ_LOG_ERROR:
      d = 'E';
      break;
    case DZ_LOG_WARNING: /* info about normal operation */
      d = 'W';
      break;
    case DZ_LOG_NOTICE:
      d = 'T';
      break;
    case DZ_LOG_INFO:
      d = 'I';
      break; /* Normal information */
    case DZ_LOG_DEBUG:
      d = 'D';
      break; /* internal errors */
    case DZ_LOG_TRACE:
      d = 'R';
      break;
    default:
      break;
  }
  return d;
}

bool
isWriteMode(int flags)
{
  bool isRW = false;
  if (flags & (O_RDWR | O_WRONLY | O_CREAT | O_TRUNC | O_APPEND)) {
    isRW = true;
  }
  return isRW;
}

int
check_file_flag(const char* filename, int flag)
{
  // 只允许以APPEND和只读模式打开文件
  if (flag != O_RDONLY && flag != (O_WRONLY | O_APPEND)) {
    return -1;
  }

  // 打开文件
  int fd = open(filename, flag);
  if (fd == -1) {
    return -1;
  }

  // 关闭文件
  close(fd);

  return 0;
}

int
is_file_read_only(const char* filename, int flags)
{
  int fd = open(filename, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  int access_mode = flags & O_ACCMODE;
  int is_read_only = (access_mode == O_RDONLY || access_mode == O_APPEND);
  close(fd);
  return is_read_only;
}

char*
buildHiddenFile(const char* filepath)
{
  char* filename = strrchr(filepath, '/');
  // If the file path does not contain slashes, that is, it is not an absolute
  // path, return NULL
  if (filename == NULL) {
    return NULL;
  }
  char* new_filepath = (char*)malloc(strlen(filepath) + 6);
  if (new_filepath == NULL) {
    dz_msg(DZ_LOG_ERROR, "malloc new_filepath for %s failed\n", filepath);
    return NULL;
  }
  strcpy(new_filepath,
         filepath); // Firstly copy the original file path into the new string
  filename = strrchr(new_filepath, '/') + 1; // Get the file name
  char* dot = ".";
  // Move the string after the filename back one bit to make room for the dot
  // "."
  memmove(filename + strlen(dot), filename, strlen(filename) + 1);
  // Add a dot "." before the filename
  memcpy(filename, dot, strlen(dot));
  strcat(new_filepath, ".DZF");
  return new_filepath;
}
/*Find the hidden HDF5 file like .XXX.DZF
 *Return 1 if it exists, return 0 if it doesn't exist, and return -1 if there
 *is an error, and return 2 if it is directory
 */
int
findhiddenFile(const char* filename)
{
  struct stat stbuf;
  int ret;
  int rc;
  char* hiddenFile = buildHiddenFile(filename);
  if (!hiddenFile) {
    return -1; // Error
  }
  ret = stat(hiddenFile, &stbuf);
  if (ret < 0) {
    if (errno == ENOENT) {
      rc = 0;
      goto out; // It doesn't exist
    } else {
      dz_msg(DZ_LOG_ERROR, "stat %s failed %s\n", hiddenFile, strerror(errno));
      rc = -1;
      goto out; // Error occurs
    }
  }

  if (S_ISDIR(stbuf.st_mode)) {
    rc = 2;
    goto out; // It is a directory
  }
  rc = 1; // It is a file and exists
out:
  if (hiddenFile)
    free(hiddenFile);
  return rc;
}

/* find the temporary file of hidden HDF5 like .XXX.DZF.tmp
 *Return 1 if it exists, return 0 if it doesn't exist, and return -1 if there
 *is an error, and return 2 if it is directory
 */
int
findTempH5File(const char* path)
{
  char* h5name = NULL;
  char* tmp_h5name = NULL;
  struct stat stbuf;
  int ret;

  if (!path) {
    errno = EINVAL;
    return -1;
  }
  h5name = buildHiddenFile(path);
  if (!h5name)
    return -1;
  tmp_h5name = (char*)malloc(strlen(h5name) + 5);
  if (!tmp_h5name) {
    ret = -1;
    goto out;
  }
  sprintf(tmp_h5name, "%s.tmp", h5name);
  ret = stat(tmp_h5name, &stbuf);
  if (ret == 0) {
    // temporary hidden H5File already existed
    ret = 1;
    goto out;
  }
out:
  if (h5name)
    free(h5name);
  if (tmp_h5name)
    free(h5name);
  return ret;
}

/* check if it is prefined path like .dsz/XXXX
 * Return 1 if it is matched
 */
int
match_sysdir(const char* path)
{
  if (!path)
    return -1;
  char* path_copy = strdup(path);
  char* dir_token = strtok(path_copy, "/");
  while (dir_token != NULL) {
    if (strcmp(dir_token, ".dzs") == 0) {
      dz_msg(DZ_LOG_TRACE, "path '%s' is protected", path);
      free(path_copy);
      errno = EPERM;
      return 1;
    }
    dir_token = strtok(NULL, "/");
  }
  free(path_copy);
  return 0;
}
/* check if it is prefined path like .XX.DZF
 * Return 1 if it is matched
 */
int
match_hiddenfile(const char* path)
{
  if (!path)
    return -1;
  const char* filename = NULL;
  filename = strrchr(path, '/');
  if (!filename)
    filename = path;
  else
    filename = strrchr(path, '/') + 1; // Get the file name
  size_t namelen = strlen(filename);
  int result = 0;
  if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0)) {
    result = 1;
    goto out;
  }
  if (namelen >= 4 && filename[0] == '.' &&
      ((strcmp(filename + namelen - 4, ".DZF") == 0) || (strcmp(filename + namelen - 8, ".DZF.tmp") == 0))) {
    result = 1;
    goto out;
  }
out:
  return result;
}