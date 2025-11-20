#ifndef _DZ_COMMON_H
#define _DZ_COMMON_H

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum
  {
    DZ_LOG_NONE,
    DZ_LOG_EMERG,
    DZ_LOG_ALERT,
    DZ_LOG_CRITICAL, /* fatal errors */
    DZ_LOG_ERROR,    /* major failures (not necessarily fatal) */
    DZ_LOG_WARNING,  /* info about normal operation */
    DZ_LOG_NOTICE,
    DZ_LOG_INFO,  /* Normal information */
    DZ_LOG_DEBUG, /* internal errors */
    DZ_LOG_TRACE, /* full trace of operation */
  } dz_loglevel_t;

  typedef pthread_mutex_t dz_lock_t;
  typedef pthread_rwlock_t dz_rwlock_t;

#define DZ_JSON_MSG_LENGTH 8192
#define DZ_SYSLOG_CEE_FORMAT                                                   \
  "@cee: {\"msg\": \"%s\", \"gf_code\": \"%u\", \"gf_message\": \"%s\"}"
#define DZ_LOG_CONTROL_FILE "/etc/dzfile/logger.conf"
#define DZ_LOG_BACKTRACE_DEPTH 5
#define DZ_LOG_BACKTRACE_SIZE 4096
#define DZ_LOG_TIMESTR_SIZE 256
#define DZ_MAX_SLOG_PAIR_COUNT 100
#define DZ_LOGBUFSZ 2048

#define MAX_PATH_LEN 1024

#define LOCK_INIT(x) pthread_mutex_init(x, 0)
#define LOCK(x) pthread_mutex_lock(x)
#define TRY_LOCK(x) pthread_mutex_trylock(x)
#define UNLOCK(x) pthread_mutex_unlock(x)
#define LOCK_DESTROY(x) pthread_mutex_destroy(x)
#define DZ_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

  typedef pthread_cond_t dz_cond_t;
#define dz_cond_destroy(x) pthread_cond_destroy(x)
#define dz_cond_init(x, y) pthread_cond_init(x, y)
#define dz_cond_broadcast() pthread_cond_broadcast()
#define dz_cond_signal(x) pthread_cond_signal(x)
#define dz_cond_wait(x, y) pthread_cond_wait(x, y)
#define dz_cond_timedwait() pthread_cond_timedwait()

#define RWLOCK_INIT(x) pthread_rwlock_init(x, NULL)
#define RDLOCK(x) pthread_rwlock_rdlock(x)
#define TRY_RDLOCK(x) pthread_rwlock_tryrdlock(x)
#define WRLOCK(x) pthread_rwlock_wrlock(x)
#define TRY_WRLOCK(x) pthread_rwlock_trywrlock(x)
#define RWLOCK_UNLOCK(x) pthread_rwlock_unlock(x)
#define RWLOCK_DESTROY(x) pthread_rwlock_destroy(x)
#define DZ_RWLOCK_INITIALIZER PTHREAD_RWLOCK_INITIALIZER

#define VALIDATE_OR_GOTO(arg, label)                                           \
  do {                                                                         \
    if (!arg) {                                                                \
      errno = EINVAL;                                                          \
      goto label;                                                              \
    }                                                                          \
  } while (0)

#define VALIDATE_PRIV(label)                                                   \
  do {                                                                         \
    if (!priv) {                                                               \
      goto label;                                                              \
    }                                                                          \
  } while (0);

#define MAKE_REAL_PATH(var, path)                                              \
  do {                                                                         \
    size_t path_len = strlen(path);                                            \
    size_t var_len = path_len + 1;                                             \
    var_len += strlen(priv->data_path);                                        \
    if (priv->path_max != -1 && var_len >= priv->path_max) {                   \
      var = alloca(path_len + 1);                                              \
      strcpy(var, (path[0] == '/') ? path + 1 : path);                         \
    } else {                                                                   \
      var = alloca(var_len);                                                   \
      if (var) {                                                               \
        strcpy(var, priv->data_path);                                          \
        strcpy(&var[strlen(priv->data_path)], path);                           \
      }                                                                        \
    }                                                                          \
  } while (0)

  /* uint8_t  max=255;
   * uint16_t max=65535;
   * uint32_t max=4294967295;
   * uint64_t max=18446744073709551615;
   * */

#define MAXUINT64 18446744073709551615

  typedef uint8_t BYTE;
  typedef uint16_t WORD;
  typedef uint32_t LONG;
  typedef uint64_t HYPER;

#define BYTESIZE 1
#define WORDSIZE 2
#define LONGSIZE 4
#define HYPERSIZE 8

#define BYTEADDR(x) (((char*)&(x)) + sizeof(BYTE) - BYTESIZE)
#define WORDADDR(x) (((char*)&(x)) + sizeof(WORD) - WORDSIZE)
#define LONGADDR(x) (((char*)&(x)) + sizeof(LONG) - LONGSIZE)
#define QUADADDR(x) (((char*)&(x)) + sizeof(QUAD) - QUADSIZE)

#define SHORT WORD
#define SHORTSIZE WORDSIZE
#define SHORTADDR WORDADDR

#define marshall_WORD marshall_SHORT
#define unmarshall_WORD unmarshall_SHORT

#define INC_PTR(ptr, n) (ptr) = (char*)(ptr) + (n)
#define DIFF_PTR(ptr, base) (char*)(ptr) - (char*)(base)

  /*
   *  *  * Allocate enough memory for a 'bitvct' type variable containing
   *   *   * 'size' bits
   *    *    */

#define bitalloc(size)                                                         \
  (bitvct) malloc(size / BITSOFBYTE + ((size % BITSOFBYTE) ? 1 : 0))

  /*
   *  *  *  Set the bit 'bit-th' starting from the byte pointed to by 'ptr'
   *   *   */

#define BIT_SET(ptr, bit)                                                      \
  {                                                                            \
    char* p = (char*)(ptr) + (bit) / 8;                                        \
    *p = *p | (1 << (7 - (bit) % 8));                                          \
  }

  /*
   *  *  *  Clear the bit 'bit-th' starting from the byte pointed to by 'ptr'
   *   *   */

#define BIT_CLR(ptr, bit)                                                      \
  {                                                                            \
    char* p = (char*)(ptr) + (bit) / 8;                                        \
    *p = *p & ~(1 << (7 - (bit) % 8));                                         \
  }

  /*
   *  *  *  Test the bit 'bit-th' starting from the byte pointed to by 'ptr'
   *   *   */

#define BIT_ISSET(ptr, bit)                                                    \
  (*(char*)((char*)(ptr) + (bit) / 8) & (1 << (7 - (bit) % 8)))

/* Interface to log messages with message IDs */
#define dz_msg(levl, fmt...)                                                   \
  do {                                                                         \
    _dz_msg(                                                                   \
      "DZFILE", __FILE__, __FUNCTION__, __LINE__, levl, errno, 0, 1, ##fmt);   \
  } while (0)

#define ENTRY() dz_msg(DZ_LOG_TRACE, "Enter ...\n")
#define END() dz_msg(DZ_LOG_TRACE, "End ...\n")

#define FAILRET()                                                              \
  do {                                                                         \
    if (errno != 0) {                                                          \
      dz_msg(DZ_LOG_ERROR, "%s\n", strerror(errno));                           \
    }                                                                          \
    goto out;                                                                  \
  } while (0)

#define PRINT_SIZE_CHECK(ret, label, strsize)                                  \
  do {                                                                         \
    if (ret < 0)                                                               \
      goto label;                                                              \
    if ((strsize - ret) > 0) {                                                 \
      strsize -= ret;                                                          \
    } else {                                                                   \
      ret = 0;                                                                 \
      goto label;                                                              \
    }                                                                          \
  } while (0)
#define SET_LOG_PRIO(level, priority)                                          \
  do {                                                                         \
    if (GF_LOG_TRACE == (level) || GF_LOG_NONE == (level)) {                   \
      priority = DZ_LOG_DEBUG;                                                 \
    } else {                                                                   \
      priority = (level)-1;                                                    \
    }                                                                          \
  } while (0)

  int make_parent_dir(const char* path);
  bool isWriteMode(int flags);

  int intstr_length(uint64_t i);

  int _dz_msg(const char* domain,
              const char* file,
              const char* function,
              int32_t line,
              dz_loglevel_t level,
              int errnum,
              int trace,
              uint64_t msgid,
              const char* fmt,
              ...);
  static int _dz_msg_backtrace(int stacksize, char* callstr, size_t strsize);
  static char _dz_log_char(int loglevel);

  char* buildHiddenFile(const char* filepath);
  int findhiddenFile(const char* filename);
  int findTempH5File(const char* path);
  int match_sysdir(const char* path);
  int match_hiddenfile(const char* path);
#ifdef __cplusplus
}
#endif
#endif
