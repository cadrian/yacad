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

#include <sqlite3.h>

#include "yacad_tasklist.h"

#define STMT_CREATE_TABLE "create table TASKLIST ("      \
     "ID integer primary key asc autoincrement, "        \
     "TIMESTAMP integer not null, "                      \
     "STATUS integer not null, "                         \
     "SER not null"                                      \
     ")"

#define STMT_SELECT "select ID, TIMESTAMP, STATUS, SER from TASKLIST where STATUS=? order by ID asc"
#define STMT_INSERT "insert into TASKLIST (TIMESTAMP, STATUS, SER) values (?,?,?)"
#define STMT_UPDATE "update TASKLIST set STATUS=? where ID = ?"

typedef struct yacad_tasklist_impl_s {
     yacad_tasklist_t fn;
     logger_t log;
     cad_array_t *tasklist;
     sqlite3 *db;
} yacad_tasklist_impl_t;

static bool_t __sqlcheck(sqlite3 *db, logger_t log, int sqlerr, level_t level, const char *sqlaction, unsigned int line) {
     int result = true;
     const char *msg;
     char *paren;
     int len;
     if (sqlerr != SQLITE_OK) {
          log(debug, "Error line %u: %s\n", line, sqlaction);
          paren = strchrnul(sqlaction, '(');
          len = paren - sqlaction;
          msg = db == NULL ? NULL : sqlite3_errmsg(db);
          if (msg == NULL) {
               log(level, "%.*s: error %d\n", len, sqlaction, sqlerr);
          } else {
               log(level, "%.*s: error %d: %s\n", len, sqlaction, sqlerr, msg);
          }
          result = false;
     }
     return result;
}

#define sqlcheck(db, log, sqlaction, level) __sqlcheck(db, log, (sqlaction), (level), #sqlaction, __LINE__)

static void add(yacad_tasklist_impl_t *this, yacad_task_t *task) {
     int i, n;
     bool_t found = false;
     yacad_task_t *other;
     sqlite3_stmt *query = NULL;
     const char *ser = task->serialize(task);
     time_t timestamp = task->get_timestamp(task);
     yacad_task_status_t status = task->get_status(task);

     n = this->tasklist->count(this->tasklist);
     for (i = 0; !found && i < n; i++) {
          other = *(yacad_task_t**)this->tasklist->get(this->tasklist, i);
          found = task->same_as(task, other);
     }
     if (found) {
          this->log(debug, "Task not added: %s\n", ser);
          task->free(task);
     } else {
          if (sqlcheck(this->db, this->log, sqlite3_prepare_v2(this->db, STMT_INSERT, -1, &query, NULL), warn)) {
               sqlcheck(this->db, this->log, sqlite3_bind_int64(query, 1, (sqlite3_int64)timestamp), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_int(query, 2, (int)status), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_text64(query, 3, ser, strlen(ser), SQLITE_TRANSIENT, SQLITE_UTF8), warn);
               sqlite3_step(query);
               task->set_id(task, (unsigned long)sqlite3_last_insert_rowid(this->db));
               sqlcheck(this->db, this->log, sqlite3_finalize(query), warn);
          }

          ser = task->serialize(task);
          this->log(info, "Added task: %s\n", ser);
          this->tasklist->insert(this->tasklist, n, &task);
     }
}

static yacad_task_t *get(yacad_tasklist_impl_t *this, yacad_runner_t *runner) {
     yacad_task_t *result = NULL;
     int index = 0; // TODO find the best matching index
     result = *(yacad_task_t **)this->tasklist->get(this->tasklist, index);
     this->tasklist->del(this->tasklist, index);
     // TODO save to database // NO!! do that only when the task is successfully performed (we need an extra method here)
     return result;
}

static void free_(yacad_tasklist_impl_t *this) {
     int i, n = this->tasklist->count(this->tasklist);
     yacad_task_t *task;
     for (i = 0; i < n; i++) {
          task = *(yacad_task_t **)this->tasklist->get(this->tasklist, i);
          task->free(task);
     }
     this->tasklist->free(this->tasklist);
     if (this->db != NULL) {
          sqlite3_close(this->db);
     }
     free(this);
}

static yacad_tasklist_t impl_fn = {
     .add = (yacad_tasklist_add_fn)add,
     .get = (yacad_tasklist_get_fn)get,
     .free = (yacad_tasklist_free_fn)free_,
};

static void add_task(yacad_tasklist_impl_t *this, sqlite3_int64 sql_id, sqlite3_int64 sql_timestamp, int sql_status, const unsigned char *sql_desc) {
     yacad_task_t *task = yacad_task_unserialize(this->log, (unsigned long)sql_id, (time_t)sql_timestamp, (yacad_task_status_t)sql_status, (char*)sql_desc);
     this->tasklist->insert(this->tasklist, this->tasklist->count(this->tasklist), &task);
     this->log(info, "Restored task: %s\n", task->serialize(task));
}

yacad_tasklist_t *yacad_tasklist_new(logger_t log, const char *database_name) {
     yacad_tasklist_impl_t *result;
     static bool_t init = false;
     sqlite3 *db = NULL;
     sqlite3_stmt *query = NULL;
     bool_t done = false;
     int err;

     if (!init) {
          if (!sqlcheck(NULL, log, sqlite3_initialize(), debug)) {
               log(warn, "Could not initialize database: %s\n", database_name);
               return NULL;
          }
          init = true;
     }

     if (!sqlcheck(NULL, log, sqlite3_open_v2(database_name, &db, SQLITE_OPEN_READWRITE, NULL), debug)) {
          if (!sqlcheck(NULL, log, sqlite3_open_v2(database_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL), debug)) {
               log(warn, "Could not create database: %s\n", database_name);
               return NULL;
          }
          if (!sqlcheck(db, log, sqlite3_exec(db, STMT_CREATE_TABLE, NULL, NULL, NULL), debug)) {
               log(warn, "Could not create table\n");
               sqlite3_close(db);
               return NULL;
          }
     }

     result = malloc(sizeof(yacad_tasklist_impl_t));
     result->fn = impl_fn;
     result->log = log;
     result->tasklist = cad_new_array(stdlib_memory, sizeof(yacad_task_t*));
     result->db = db;

     if (sqlcheck(db, log, sqlite3_prepare_v2(db, STMT_SELECT, -1, &query, NULL), warn)) {
          sqlcheck(db, log, sqlite3_bind_int(query, 1, (int)task_new), warn);
          do {
               err = sqlite3_step(query);
               switch(err) {
               case SQLITE_DONE:
                    done = true;
                    break;
               case SQLITE_BUSY:
                    log(info, "Database is busy, waiting one second\n");
                    sleep(1);
                    break;
               case SQLITE_OK: // ??
               case SQLITE_ROW:
                    add_task(result,
                             sqlite3_column_int64(query, 0),
                             sqlite3_column_int64(query, 1),
                             sqlite3_column_int(query, 2),
                             sqlite3_column_text(query, 3));
                    break;
               default:
                    sqlcheck(db, log, err, warn);
                    done = true;
               }
          } while (!done);

          sqlcheck(db, log, sqlite3_finalize(query), warn);
     }

     return I(result);
}
