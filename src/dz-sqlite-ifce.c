#include "dzfile.h"
#include "sqlite-ifce.h"
#include <assert.h>
#include <errno.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define TRY_TIMES 10

int
dz_opendb()
{
  int ret = -1;
  char* dbfile = NULL;
  struct stat stbuf;
  struct dzfile_priv* priv = get_priv();
  VALIDATE_PRIV(out);

  MAKE_REAL_PATH(dbfile, "/.dzs/meta.db");
  if (!dbfile) {
    dz_msg(DZ_LOG_ERROR, "%s\n", strerror(errno));
    goto out;
  }
  ret = make_parent_dir(dbfile);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR, "%s\n", strerror(errno));
    goto out;
  }
  ret = lstat(dbfile, &stbuf);
  if (ret < 0) {
    dz_msg(DZ_LOG_ERROR,
           "Faild to stat dbfile (%s), please initialize it firstly!!!\n",
           dbfile);
    goto out;
  }
  sqlite3_initialize();
  ret = sqlite3_open(dbfile, &priv->db);
  if (ret) {
    dz_msg(DZ_LOG_ERROR, "Can't open database: %s\n", sqlite3_errmsg(priv->db));
    ret = -1;
    goto out;
  }

  ret = sqlite3_busy_timeout(priv->db, 1000);
  if (SQLITE_OK != ret) {
    dz_msg(
      DZ_LOG_ERROR, "Can't set busy handle: %s\n", sqlite3_errmsg(priv->db));
    ret = -1;
    goto out;
  }

  ret = 0;
out:
  return ret;
}

int
dz_closedb()
{
  struct dzfile_priv* priv = get_priv();
  VALIDATE_PRIV(out);
  VALIDATE_OR_GOTO(priv->db, out);
  sqlite3_close(priv->db);
  sqlite3_shutdown();
out:
  return -1;
}

/* insert a file entry
 * return 0 on success, -1 on failure
 * */

int
fmt_entry_insert(const char* path)
{
  char sql[1024];
  time_t insert_time;

  static char insert_stmt[] = "INSERT INTO filemeta(path, ctime) \
                    VALUES('%s', %d);";

  int rc = -1;
  char* errmsg = 0;
  struct dzfile_priv* priv = get_priv();

  VALIDATE_PRIV(out);
  VALIDATE_OR_GOTO(priv->db, out);
  sqlite3* db = priv->db;
  time(&insert_time);
  sprintf(sql, insert_stmt, path, insert_time);
  LOCK(&priv->dblock);
  rc = sqlite3_exec(db, sql, NULL, 0, &errmsg);
  if (rc != SQLITE_OK) {
    if (rc == SQLITE_FULL) {
      dz_msg(DZ_LOG_ERROR,
             "id already reached max value: 9223372036854775807\n");
    }
    dz_msg(DZ_LOG_ERROR, "SQL error: %s\n", errmsg);
    sqlite3_free(errmsg);
    UNLOCK(&priv->dblock);
    return -1;
  }
  UNLOCK(&priv->dblock);
  rc = add_filenum(1);
out:
  return rc;
}

/* delete a file entry
 * return 0 on success, -1 on failure
 * */
int
fmt_entry_delete(const char* path)
{
  char sql[1024] = { 0 };
  static char delete_stmt[] = "DELETE FROM filemeta where path='%s';";
  int rc = -1;
  char* zErrMsg = 0;
  struct dzfile_priv* priv = get_priv();

  VALIDATE_PRIV(out);
  VALIDATE_OR_GOTO(priv->db, out);
  sqlite3* db = priv->db;

  sprintf(sql, delete_stmt, path);
  LOCK(&priv->dblock);
  rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    dz_msg(DZ_LOG_CRITICAL, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    UNLOCK(&priv->dblock);
    return -1;
  }
  UNLOCK(&priv->dblock);
  rc = sub_filenum(1);
out:
  return rc;
}

/*update bytes in space table
 *If flag is ADD_NUM, num will be added into writtenbytes or diskbytes or filenum
 *If flag is SUB_NUM, num will be subtracted writtenbytes or diskbytes or filenum
 */
static int
space_update(const char* field, ssize_t num, int flag)
{
  char sql[1024] = { 0 };
  static char update_stmt[] = "update space set %s = %s %c %llu;";
  int rc = -1;
  char* zErrMsg = 0;
  struct dzfile_priv* priv = get_priv();
  char c = '-';

  VALIDATE_PRIV(out);
  VALIDATE_OR_GOTO(priv->db, out);
  sqlite3* db = priv->db;

  switch (flag) {
    case ADD_NUM:
      c = '+';
      break;
    case SUB_NUM:
      c = '-';
      break;
    default:
      rc = -1;
      goto out;
  }
  sprintf(sql, update_stmt, field, field, c, num);
  LOCK(&priv->dblock);
  rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
  if (rc != SQLITE_OK) {
    dz_msg(DZ_LOG_CRITICAL, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
    UNLOCK(&priv->dblock);
    return -1;
  }
  UNLOCK(&priv->dblock);
out:
  return rc;
}

int
add_writtenbytes(ssize_t nbytes)
{
  return space_update("writtenbytes", nbytes, ADD_NUM);
}
int
sub_writtenbytes(ssize_t nbytes)
{
  return space_update("writtenbytes", nbytes, SUB_NUM);
}
int
add_diskbytes(ssize_t nbytes)
{
  return space_update("diskbytes", nbytes, ADD_NUM);
}
int
sub_diskbytes(ssize_t nbytes)
{
  return space_update("diskbytes", nbytes, SUB_NUM);
}

int
add_filenum(ssize_t num)
{
  return space_update("filenum", num, ADD_NUM);
}
int
sub_filenum(ssize_t num)
{
  return space_update("filenum", num, SUB_NUM);
}

/* get a record from space table */
int get_space_entry(struct space_entry *spentry)
{
  int nrow, ncolumn;
  char** azresult;
  char* errmsg;
  char sql[1024] = "select writtenbytes, diskbytes, filenum from space where id=1;";
  struct dzfile_priv* priv = get_priv();
  VALIDATE_PRIV(out);
  VALIDATE_OR_GOTO(priv->db, out);
  sqlite3* db = priv->db;
  if (!spentry)
    return -1;

  LOCK(&priv->dblock);
  if (SQLITE_OK !=
      sqlite3_get_table(db, sql, &azresult, &nrow, &ncolumn, &errmsg)) {
    dz_msg(DZ_LOG_ERROR, "get space table failed : %s\n", errmsg);
    sqlite3_free(errmsg);
    UNLOCK(&priv->dblock);
    return -1;
  }
  if (nrow == 0) {
    UNLOCK(&priv->dblock);
    dz_msg(DZ_LOG_CRITICAL, "Database is empty \n");
    errno = ENOENT;
    return -1;
  }
  if (azresult[3] != NULL) {
    spentry->writtenbytes = (uint64_t)strtoll(azresult[3], NULL, 0);
  }
  if (azresult[4] != NULL) {
    spentry->diskbytes = (uint64_t)strtoll(azresult[4], NULL, 0);
  }
  if (azresult[5] != NULL) {
    spentry->filenum = (uint64_t)strtoll(azresult[5], NULL, 0);
  }
  UNLOCK(&priv->dblock);
out:
  if (azresult) {
    sqlite3_free_table(azresult);
    azresult = NULL;
  }
  if (priv && priv->db) {
    UNLOCK(&priv->dblock);
  }
  return 0;
}
