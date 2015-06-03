/*
  This file is part of yaCAD.

  yaCAD is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, version 3 of the License.

  yaCAD is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with yaCAD.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "test.h"
#include "mocks.h"

#include "common/database/yacad_database_sqlite3.h"

static const char* mock_sqlite3_errmsg(sqlite3* db) {
     return result(const char*);
}

static int mock_sqlite3_bind_int64(sqlite3_stmt *stmt, int index, sqlite3_int64 value) {
     check(index);
     check(value);
     return result(int);
}

static int mock_sqlite3_bind_text64(sqlite3_stmt *stmt, int index, const char *value, sqlite3_uint64 nbytes, void(*del)(void*), unsigned char encoding) {
     check(index);
     check(value);
     check(nbytes);
     check(del);
     check(encoding);
     return result(int);
}

static int mock_sqlite3_step(sqlite3_stmt *stmt) {
     return result(int);
}

static sqlite3_int64 mock_sqlite3_last_insert_rowid(sqlite3 *db) {
     return result(sqlite3_int64);
}

static sqlite3_int64 mock_sqlite3_column_int64(sqlite3_stmt *stmt, int index) {
     check(index);
     return result(sqlite3_int64);
}

static const unsigned char *mock_sqlite3_column_text(sqlite3_stmt *stmt, int index) {
     check(index);
     return result(const unsigned char *);
}

static int mock_sqlite3_finalize(sqlite3_stmt *stmt) {
     return result(int);
}

static int mock_sqlite3_prepare_v2(sqlite3 *db, const char *query, int bytes, sqlite3_stmt **stmt, const char **tail) {
     int n;
     check(query);
     check(bytes);
     check(stmt);
     check(tail);
     if (bytes != 0) {
          n = strlen(query);
          if (bytes < 0) {
               bytes = n;
          } else if (bytes > n) {
               bytes = n;
          }
          if (stmt) {
               *stmt = malloc(1);
               memset(*stmt, 0, 1);
          }
          if (tail) {
               *tail = query + bytes;
          }
     }
     return result(int);
}

static int mock_sqlite3_close(sqlite3 *db) {
     return result(int);
}

static int mock_sqlite3_initialize(void) {
     return result(int);
}

static int mock_sqlite3_open_v2(const char *filename, sqlite3 **db, int flags, const char *vfs) {
     check(filename);
     check(db);
     check(flags);
     check(vfs);
     if (db != NULL) {
          *db = malloc(1);
     }
     return result(int);
}

static int mock_sqlite3_exec(sqlite3 *db, const char *query, int (*callback)(void*,int,char**,char**), void *arg, char **errmsg) {
     check(query);
     check(callback);
     check(arg);
     check(errmsg);
     return result(int);
}

static sqlite3_fn_t mock_sqlite3 = {
     .errmsg            = mock_sqlite3_errmsg           ,
     .bind_int64        = mock_sqlite3_bind_int64       ,
     .bind_text64       = mock_sqlite3_bind_text64      ,
     .step              = mock_sqlite3_step             ,
     .last_insert_rowid = mock_sqlite3_last_insert_rowid,
     .column_int64      = mock_sqlite3_column_int64     ,
     .column_text       = mock_sqlite3_column_text      ,
     .finalize          = mock_sqlite3_finalize         ,
     .prepare           = mock_sqlite3_prepare_v2       ,
     .close             = mock_sqlite3_close            ,
     .initialize        = mock_sqlite3_initialize       ,
     .open              = mock_sqlite3_open_v2          ,
     .exec              = mock_sqlite3_exec             ,
};

int test(void) {
     int result = 0;
     logger_t log = get_logger(trace);
     yacad_database_t *db;

     yacad_database_set_sqlite_fn(mock_sqlite3);

     //push_result(mock_sqlite3_errmsg, "mock_sqlite3_errmsg");
     push_result(mock_sqlite3_initialize, SQLITE_OK);

     expect_string(mock_sqlite3_open_v2, filename, "TESTDB");

     db = yacad_database_new(log, "TESTDB");
     db->free(db);

     verify_mocks();

     return result;
}
