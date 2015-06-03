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

#include "yacad_database_sqlite3.h"

#define DB_VERSION 1L

#define STMT_CREATE_TABLE "create table PARAM (KEY not null, VALUE not null)"
#define STMT_SELECT "select VALUE from PARAM where KEY=?"
#define STMT_INSERT "insert into PARAM (KEY,VALUE) values (?,?)"

static sqlite3_fn_t sqlite3_fn = {
     .errmsg              = sqlite3_errmsg           ,
     .bind_int64          = sqlite3_bind_int64       ,
     .bind_text64         = sqlite3_bind_text64      ,
     .step                = sqlite3_step             ,
     .last_insert_rowid   = sqlite3_last_insert_rowid,
     .column_int64        = sqlite3_column_int64     ,
     .column_text         = sqlite3_column_text      ,
     .finalize            = sqlite3_finalize         ,
     .prepare             = sqlite3_prepare_v2       ,
     .close               = sqlite3_close            ,
     .initialize          = sqlite3_initialize       ,
     .open                = sqlite3_open_v2          ,
     .exec                = sqlite3_exec             ,
};

static bool_t __sqlcheck(sqlite3 *db, logger_t log, sqlite3_fn_t *sql_fn, int sqlerr, level_t level, const char *sqlaction, unsigned int line) {
     int result = true;
     const char *msg;
     char *paren;
     int len;
     if (sqlerr != SQLITE_OK) {
          log(debug, "Error line %u: %s", line, sqlaction);
          paren = strchrnul(sqlaction, '(');
          len = paren - sqlaction;
          msg = db == NULL ? NULL : sql_fn->errmsg(db);
          if (msg == NULL) {
               log(level, "%.*s: error %d", len, sqlaction, sqlerr);
          } else {
               log(level, "%.*s: error %d: %s", len, sqlaction, sqlerr, msg);
          }
          result = false;
     }
     return result;
}

#define sqlcheck0(db, log, sqlaction, level) __sqlcheck((db), (log), &sqlite3_fn, (sqlaction), (level), #sqlaction, __LINE__)
#define sqlcheck(this, sqlaction, level) __sqlcheck((this)->db, (this)->log, (this)->sql_fn, (sqlaction), (level), #sqlaction, __LINE__)

typedef struct yacad_database_impl_s {
     yacad_database_t fn;
     logger_t log;
     sqlite3 *db;
     sqlite3_fn_t *sql_fn; // points to sql_fn_copy
     sqlite3_fn_t sql_fn_copy;
} yacad_database_impl_t;

typedef struct yacad_statement_impl_s {
     yacad_statement_t fn;
     logger_t log;
     sqlite3 *db;
     sqlite3_stmt *query;
     long rowid;
     sqlite3_fn_t *sql_fn;
} yacad_statement_impl_t;

static void bind_int(yacad_statement_impl_t *this, int index, long value) {
     this->log(trace, " - bind int #%d: %ld", index, value);
     sqlcheck(this, this->sql_fn->bind_int64(this->query, index + 1, (sqlite3_int64)value), warn);
}

static void bind_string(yacad_statement_impl_t *this, int index, const char *value) {
     this->log(trace, " - bind string #%d: '%s'", index, value);
     sqlcheck(this, this->sql_fn->bind_text64(this->query, index + 1, value, strlen(value), SQLITE_TRANSIENT, SQLITE_UTF8), warn);
}

static void select_run(yacad_statement_impl_t *this, yacad_select_fn select, void *data) {
     bool_t done = false;
     int err;

     this->log(trace, " - executing select");
     do {
          err = this->sql_fn->step(this->query);
          switch(err) {
          case SQLITE_DONE:
               done = true;
               break;
          case SQLITE_BUSY:
               this->log(info, "Database is busy, waiting one second");
               sleep(1);
               break;
          case SQLITE_OK: // ??
          case SQLITE_ROW:
               if (select != NULL) {
                    select(I(this), data);
               }
               break;
          default:
               sqlcheck(this, err, error);
               done = true;
          }
     } while (!done);
}

static void update_run(yacad_statement_impl_t *this, yacad_select_fn select, void *data) {
     bool_t done = false;
     int err;

     this->log(trace, " - executing update");
     do {
          err = this->sql_fn->step(this->query);
          switch(err) {
          case SQLITE_DONE:
               this->rowid = (long)this->sql_fn->last_insert_rowid(this->db);
               done = true;
               break;
          case SQLITE_BUSY:
               this->log(info, "Database is busy, waiting one second");
               sleep(1);
               break;
          case SQLITE_OK: // ??
          case SQLITE_ROW:
               this->rowid = (long)this->sql_fn->last_insert_rowid(this->db);
               if (select != NULL) {
                    select(I(this), data);
               }
               break;
          default:
               sqlcheck(this, err, error);
               done = true;
          }
     } while (!done);
}

static long get_rowid(yacad_statement_impl_t *this) {
     return this->rowid;
}

static long get_int(yacad_statement_impl_t *this, int index) {
     long result = (long)this->sql_fn->column_int64(this->query, index);
     this->log(trace, " - int #%d = %ld", index, result);
     return result;
}

static const char *get_string(yacad_statement_impl_t *this, int index) {
     char *result = (char *)this->sql_fn->column_text(this->query, index);
     this->log(trace, " - string #%d = %s", index, result);
     return result;
}

static void free__(yacad_statement_impl_t *this) {
     sqlcheck(this, this->sql_fn->finalize(this->query), warn);
     free(this);
}

static yacad_statement_t select_fn = {
     .bind_int = (yacad_statement_bind_int_fn)bind_int,
     .bind_string = (yacad_statement_bind_string_fn)bind_string,
     .run = (yacad_statement_run_fn)select_run,
     .get_rowid = NULL,
     .get_int = (yacad_statement_get_int_fn)get_int,
     .get_string = (yacad_statement_get_string_fn)get_string,
     .free = (yacad_statement_free_fn)free__,
};

static yacad_statement_t update_fn = {
     .bind_int = (yacad_statement_bind_int_fn)bind_int,
     .bind_string = (yacad_statement_bind_string_fn)bind_string,
     .run = (yacad_statement_run_fn)update_run,
     .get_rowid = (yacad_statement_get_rowid_fn)get_rowid,
     .get_int = (yacad_statement_get_int_fn)get_int,
     .get_string = (yacad_statement_get_string_fn)get_string,
     .free = (yacad_statement_free_fn)free__,
};

static void _need_install(yacad_statement_t *this, bool_t *result) {
     long ver = this->get_int(this, 0);
     if (ver == DB_VERSION) {
          *result = false;
     }
}

static bool_t need_install(yacad_database_impl_t *this) {
     bool_t result = true;
     yacad_statement_t *stmt = I(this)->select(I(this), STMT_SELECT);
     if (stmt != NULL) {
          stmt->bind_string(stmt, 0, "dbversion");
          stmt->run(stmt, (yacad_select_fn)_need_install, &result);
          stmt->free(stmt);
     }
     return result;
}

static void set_installed(yacad_database_impl_t *this) {
     yacad_statement_t *stmt = I(this)->update(I(this), STMT_INSERT);
     if (stmt != NULL) {
          stmt->bind_string(stmt, 0, "dbversion");
          stmt->bind_int(stmt, 1, DB_VERSION);
          stmt->run(stmt, NULL, NULL);
          stmt->free(stmt);
     }
}

static yacad_statement_impl_t *new_statement(yacad_database_impl_t *this, const char *statement) {
     yacad_statement_impl_t *result;
     sqlite3_stmt *query = NULL;
     if (!sqlcheck(this, this->sql_fn->prepare(this->db, statement, -1, &query, NULL), error)) {
          return NULL;
     }
     this->log(debug, "%s", statement);
     result = malloc(sizeof(yacad_statement_impl_t));
     result->log = this->log;
     result->db = this->db;
     result->query = query;
     result->sql_fn = this->sql_fn;
     return result;
}

static yacad_statement_t *select_(yacad_database_impl_t *this, const char *statement) {
     yacad_statement_impl_t *result = new_statement(this, statement);
     result->fn = select_fn;
     return I(result);
}

static yacad_statement_t *update(yacad_database_impl_t *this, const char *statement) {
     yacad_statement_impl_t *result = new_statement(this, statement);
     result->fn = update_fn;
     return I(result);
}

static void free_(yacad_database_impl_t *this) {
     this->sql_fn->close(this->db);
     free(this);
}

static yacad_database_t impl_fn = {
     .need_install = (yacad_database_need_install_fn)need_install,
     .set_installed = (yacad_database_set_installed_fn)set_installed,
     .select = (yacad_database_select_fn)select_,
     .update = (yacad_database_update_fn)update,
     .free = (yacad_database_free_fn)free_,
};

yacad_database_t *yacad_database_new(logger_t log, const char *database_name) {
     yacad_database_impl_t *result = malloc(sizeof(yacad_database_impl_t));
     static bool_t init = false;
     sqlite3 *db;

     if (!init) {
          if (!sqlcheck0(NULL, log, sqlite3_fn.initialize(), debug)) {
               log(error, "Could not initialize database: %s", database_name);
               return NULL;
          }
          init = true;
     }

     if (!sqlcheck0(NULL, log, sqlite3_fn.open(database_name, &db, SQLITE_OPEN_READWRITE, NULL), debug)) {
          log(debug, "Creating database: %s", database_name);
          if (!sqlcheck0(NULL, log, sqlite3_fn.open(database_name, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL), debug)) {
               log(error, "Could not create database: %s", database_name);
               return NULL;
          }
          log(debug, "Creating table: PARAM");
          if (!sqlcheck0(db, log, sqlite3_fn.exec(db, STMT_CREATE_TABLE, NULL, NULL, NULL), debug)) {
               log(error, "Could not create table");
               sqlite3_close(db);
               return NULL;
          }
     }

     result->fn = impl_fn;
     result->log = log;
     result->db = db;
     result->sql_fn_copy = sqlite3_fn;
     result->sql_fn = &(result->sql_fn_copy);

     return I(result);
}

void yacad_database_set_sqlite_fn(sqlite3_fn_t fn) {
     sqlite3_fn = fn;
}
