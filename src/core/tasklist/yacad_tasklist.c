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

#define STMT_CREATE_TABLE "create table TASKLIST ("       \
     "ID integer primary key asc autoincrement, "         \
     "TIMESTAMP integer not null, "                       \
     "STATUS integer not null, "                          \
     "SOURCE not null,"                                   \
     "RUN not null,"                                      \
     "RUNNERID not null,"                                 \
     "PROJECT not null,"                                  \
     "TASKINDEX not null"                                 \
     ")"

#define STMT_SELECT "select ID, TIMESTAMP, STATUS, SOURCE, RUN, RUNNERID, PROJECT, TASKINDEX from TASKLIST where STATUS=? order by ID asc"
#define STMT_INSERT "insert into TASKLIST (TIMESTAMP, STATUS, SOURCE, RUN, RUNNERID, PROJECT, TASKINDEX) values (?,?,?,?,?,?,?)"
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
          log(debug, "Error line %u: %s", line, sqlaction);
          paren = strchrnul(sqlaction, '(');
          len = paren - sqlaction;
          msg = db == NULL ? NULL : sqlite3_errmsg(db);
          if (msg == NULL) {
               log(level, "%.*s: error %d", len, sqlaction, sqlerr);
          } else {
               log(level, "%.*s: error %d: %s", len, sqlaction, sqlerr, msg);
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
     yacad_runnerid_t *runnerid = task->get_runnerid(task);
     json_value_t *jsource = task->get_source(task);
     json_value_t *jrun = task->get_run(task);
     char *src;
     char *run;
     const char *rid;
     const char *prj;
     time_t timestamp = task->get_timestamp(task);
     yacad_task_status_t status = task->get_status(task);
     int taskindex = task->get_taskindex(task);
     json_output_stream_t *out;
     json_visitor_t *writer;

     n = this->tasklist->count(this->tasklist);
     for (i = 0; !found && i < n; i++) {
          other = *(yacad_task_t**)this->tasklist->get(this->tasklist, i);
          found = task->same_as(task, other);
     }
     if (found) {
          task->free(task);
     } else {
          out = new_json_output_stream_from_string(&src, stdlib_memory);
          writer = json_write_to(out, stdlib_memory, 0);
          jsource->accept(jsource, writer);
          writer->free(writer);
          out->free(out);

          out = new_json_output_stream_from_string(&run, stdlib_memory);
          writer = json_write_to(out, stdlib_memory, 0);
          jrun->accept(jrun, writer);
          writer->free(writer);
          out->free(out);

          rid = runnerid->serialize(runnerid);

          prj = task->get_project_name(task);

          if (sqlcheck(this->db, this->log, sqlite3_prepare_v2(this->db, STMT_INSERT, -1, &query, NULL), error)) {
               sqlcheck(this->db, this->log, sqlite3_bind_int64(query, 1, (sqlite3_int64)timestamp), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_int(query, 2, (int)status), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_text64(query, 3, src, strlen(src), SQLITE_TRANSIENT, SQLITE_UTF8), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_text64(query, 4, run, strlen(run), SQLITE_TRANSIENT, SQLITE_UTF8), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_text64(query, 5, rid, strlen(rid), SQLITE_TRANSIENT, SQLITE_UTF8), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_text64(query, 6, prj, strlen(prj), SQLITE_TRANSIENT, SQLITE_UTF8), warn);
               sqlcheck(this->db, this->log, sqlite3_bind_int(query, 7, taskindex), warn);
               sqlite3_step(query);
               task->set_id(task, (unsigned long)sqlite3_last_insert_rowid(this->db));
               sqlcheck(this->db, this->log, sqlite3_finalize(query), warn);
          }

          this->log(info, "Added task: {\"id\":%lu,\"runnerid\":%s,\"source\":%s,\"run\":%s,\"project\":\"%s\",\"taskindex\":%d}", task->get_id(task), rid, src, run, prj, taskindex);
          this->tasklist->insert(this->tasklist, n, &task);

          free(src);
          free(run);
     }
}

static yacad_task_t *get(yacad_tasklist_impl_t *this, yacad_runnerid_t *runnerid) {
     yacad_task_t *result = NULL, *task;
     unsigned int best_index, index, count = this->tasklist->count(this->tasklist);
     int best_match = -1, match;
     yacad_runnerid_t *task_runnerid;
     for (index = 0; result == NULL && index < count; index++) {
          task = *(yacad_task_t **)this->tasklist->get(this->tasklist, index);
          if (task->get_status(task) == task_new) {
               task_runnerid = task->get_runnerid(task);
               match = runnerid->match(runnerid, task_runnerid);
               if (match > best_match) {
                    best_match = match;
                    best_index = index;
               }
          }
     }
     if (best_match >= 0) {
          result = *(yacad_task_t **)this->tasklist->get(this->tasklist, best_index);
          this->tasklist->del(this->tasklist, best_index);
     }
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

static void add_task(yacad_tasklist_impl_t *this, sqlite3_int64 sql_id, sqlite3_int64 sql_timestamp, int sql_status,
                     const unsigned char *sql_source, const unsigned char *sql_run, const unsigned char *sql_runnerid,
                     const unsigned char *sql_project, int sql_taskindex) {
     yacad_task_t *task;
     json_value_t *jrun;
     json_value_t *jsource;
     yacad_runnerid_t *runnerid;
     json_input_stream_t *in;

     in = new_json_input_stream_from_string((char*)sql_source, stdlib_memory);
     jsource = json_parse(in, NULL, stdlib_memory);
     in->free(in);

     in = new_json_input_stream_from_string((char*)sql_run, stdlib_memory);
     jrun = json_parse(in, NULL, stdlib_memory);
     in->free(in);

     runnerid = yacad_runnerid_unserialize(this->log, (char*)sql_runnerid);

     task = yacad_task_restore(this->log, (unsigned long)sql_id, (time_t)sql_timestamp, (yacad_task_status_t)sql_status, jrun, jsource, runnerid, (char*)sql_project, sql_taskindex);

     this->tasklist->insert(this->tasklist, this->tasklist->count(this->tasklist), &task);
     this->log(info, "Restored task: {\"id\":%lu,\"runnerid\":%s,\"source\":%s,\"run\":%s,\"project\":\"%s\",\"taskindex\":%d}", (unsigned long)sql_id, sql_runnerid, sql_source, sql_run, sql_project, sql_taskindex);
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
               log(error, "Could not initialize database: %s", database_name);
               return NULL;
          }
          init = true;
     }

     if (!sqlcheck(NULL, log, sqlite3_open_v2(database_name, &db, SQLITE_OPEN_READWRITE, NULL), debug)) {
          if (!sqlcheck(NULL, log, sqlite3_open_v2(database_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL), debug)) {
               log(error, "Could not create database: %s", database_name);
               return NULL;
          }
          if (!sqlcheck(db, log, sqlite3_exec(db, STMT_CREATE_TABLE, NULL, NULL, NULL), debug)) {
               log(error, "Could not create table");
               sqlite3_close(db);
               return NULL;
          }
     }

     result = malloc(sizeof(yacad_tasklist_impl_t));
     result->fn = impl_fn;
     result->log = log;
     result->tasklist = cad_new_array(stdlib_memory, sizeof(yacad_task_t*));
     result->db = db;

     if (sqlcheck(db, log, sqlite3_prepare_v2(db, STMT_SELECT, -1, &query, NULL), error)) {
          sqlcheck(db, log, sqlite3_bind_int(query, 1, (int)task_new), warn);
          do {
               err = sqlite3_step(query);
               switch(err) {
               case SQLITE_DONE:
                    done = true;
                    break;
               case SQLITE_BUSY:
                    log(info, "Database is busy, waiting one second");
                    sleep(1);
                    break;
               case SQLITE_OK: // ??
               case SQLITE_ROW:
                    add_task(result,
                             sqlite3_column_int64(query, 0),
                             sqlite3_column_int64(query, 1),
                             sqlite3_column_int(query, 2),
                             sqlite3_column_text(query, 3),
                             sqlite3_column_text(query, 4),
                             sqlite3_column_text(query, 5),
                             sqlite3_column_text(query, 6),
                             sqlite3_column_int(query, 7));
                    break;
               default:
                    sqlcheck(db, log, err, error);
                    done = true;
               }
          } while (!done);

          sqlcheck(db, log, sqlite3_finalize(query), warn);
     }

     return I(result);
}
