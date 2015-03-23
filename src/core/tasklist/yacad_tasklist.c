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

#include "yacad_tasklist.h"

#define STMT_DROP_TABLE "drop table if exists TASKLIST"

#define STMT_CREATE_TABLE "create table TASKLIST ("       \
     "ID integer primary key asc autoincrement, "         \
     "STATUS integer not null, "                          \
     "SERIAL not null"                                    \
     ")"

#define STMT_SELECT "select ID, STATUS, SERIAL from TASKLIST where STATUS=? order by ID asc"
#define STMT_INSERT "insert into TASKLIST (STATUS,SERIAL) values (?,?)"
#define STMT_UPDATE "update TASKLIST set STATUS=? where ID=?"

typedef struct yacad_tasklist_impl_s {
     yacad_tasklist_t fn;
     logger_t log;
     cad_array_t *tasklist;
     yacad_database_t *db;
} yacad_tasklist_impl_t;

static void add(yacad_tasklist_impl_t *this, yacad_task_t *task) {
     int i, n;
     bool_t found = false;
     yacad_task_t *other;
     yacad_statement_t *query = NULL;
     yacad_task_status_t status = task->get_status(task);
     char *serial;

     n = this->tasklist->count(this->tasklist);
     for (i = 0; !found && i < n; i++) {
          other = *(yacad_task_t**)this->tasklist->get(this->tasklist, i);
          found = task->same_as(task, other);
     }
     if (found) {
          task->free(task);
     } else {
          serial = task->serialize(task);
          query = this->db->update(this->db, STMT_INSERT);
          if (query == NULL) {
               this->log(warn, "LOST task: %s", serial);
          } else {
               query->bind_int(query, 0, status);
               query->bind_string(query, 1, serial);
               query->run(query, NULL, NULL);
               task->set_id(task, query->get_rowid(query));
               query->free(query);

               free(serial);
               serial = task->serialize(task); // to get the right id

               this->log(info, "Added task: %s", serial);
               this->tasklist->insert(this->tasklist, n, &task);
          }

          free(serial);
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

static void update_task_status(yacad_tasklist_impl_t *this, yacad_task_t *task, yacad_task_status_t status) {
     yacad_statement_t *query = NULL;
     char *serial;
     unsigned long id = task->get_id(task);

     query = this->db->update(this->db, STMT_UPDATE);
     if (query != NULL) {
          query->bind_int(query, 0, status);
          query->bind_int(query, 1, id);
          query->run(query, NULL, NULL);
          query->free(query);

          serial = task->serialize(task);
          this->log(info, "Updated task: %s", serial);
          free(serial);
     }
}

static void set_task_aborted(yacad_tasklist_impl_t *this, yacad_task_t *task) {
     update_task_status(this, task, task_aborted);
}

static void set_task_done(yacad_tasklist_impl_t *this, yacad_task_t *task) {
     update_task_status(this, task, task_done);
}

static void free_(yacad_tasklist_impl_t *this) {
     int i, n = this->tasklist->count(this->tasklist);
     yacad_task_t *task;
     for (i = 0; i < n; i++) {
          task = *(yacad_task_t **)this->tasklist->get(this->tasklist, i);
          task->free(task);
     }
     this->tasklist->free(this->tasklist);
     free(this);
}

static yacad_tasklist_t impl_fn = {
     .add = (yacad_tasklist_add_fn)add,
     .get = (yacad_tasklist_get_fn)get,
     .set_task_aborted = (yacad_tasklist_set_task_aborted_fn)set_task_aborted,
     .set_task_done = (yacad_tasklist_set_task_done_fn)set_task_done,
     .free = (yacad_tasklist_free_fn)free_,
};

static void add_task(yacad_statement_t *stmt, yacad_tasklist_impl_t *this) {
     long sql_id = stmt->get_int(stmt, 0);
     long sql_status = stmt->get_int(stmt, 1);
     const char *sql_serial = stmt->get_string(stmt, 2);
     yacad_task_t *task = yacad_task_unserialize(this->log, (char*)sql_serial);
     char *serial;
     task->set_id(task, (unsigned long)sql_id);
     task->set_status(task, (yacad_task_status_t)sql_status);
     this->tasklist->insert(this->tasklist, this->tasklist->count(this->tasklist), &task);
     serial = task->serialize(task);
     this->log(info, "Restored task: %s", serial);
     free(serial);
}

yacad_tasklist_t *yacad_tasklist_new(logger_t log, yacad_database_t *database) {
     yacad_tasklist_impl_t *result;
     yacad_statement_t *stmt;

     result = malloc(sizeof(yacad_tasklist_impl_t));
     result->fn = impl_fn;
     result->log = log;
     result->tasklist = cad_new_array(stdlib_memory, sizeof(yacad_task_t*));
     result->db = database;

     if (database->need_install(database)) {
          stmt = database->update(database, STMT_DROP_TABLE);
          if (stmt != NULL) {
               stmt->run(stmt, NULL, NULL);
               stmt->free(stmt);
          }
          stmt = database->update(database, STMT_CREATE_TABLE);
          if (stmt != NULL) {
               stmt->run(stmt, NULL, NULL);
               stmt->free(stmt);
          }
          database->set_installed(database);
     }

     stmt = database->select(database, STMT_SELECT);
     if (stmt == NULL) {
          log(warn, "Could not restore tasks");
     } else {
          stmt->bind_int(stmt, 0, task_new);
          stmt->run(stmt, (yacad_select_fn)add_task, result);
          stmt->free(stmt);
     }

     return I(result);
}
