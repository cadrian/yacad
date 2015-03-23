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

#ifndef __YACAD_DATABASE_H__
#define __YACAD_DATABASE_H__

#include "yacad.h"

typedef struct yacad_database_s yacad_database_t;
typedef struct yacad_statement_s yacad_statement_t;

typedef bool_t (*yacad_database_need_install_fn)(yacad_database_t *this);
typedef void (*yacad_database_set_installed_fn)(yacad_database_t *this);
typedef yacad_statement_t *(*yacad_database_select_fn)(yacad_database_t *this, const char *statement);
typedef yacad_statement_t *(*yacad_database_update_fn)(yacad_database_t *this, const char *statement);
typedef void (*yacad_database_free_fn)(yacad_database_t *this);

struct yacad_database_s {
     yacad_database_need_install_fn need_install;
     yacad_database_set_installed_fn set_installed;
     yacad_database_select_fn select;
     yacad_database_update_fn update;
     yacad_database_free_fn free;
};

typedef void (*yacad_select_fn)(yacad_statement_t *this, void *data);

/* Contrarily to sqlite, my column indexes always start at 0. */
typedef void (*yacad_statement_bind_int_fn)(yacad_statement_t *this, int index, long value);
typedef void (*yacad_statement_bind_string_fn)(yacad_statement_t *this, int index, const char *value);
typedef void (*yacad_statement_run_fn)(yacad_statement_t *this, yacad_select_fn select, void *data);
typedef long (*yacad_statement_get_rowid_fn)(yacad_statement_t *this);
typedef long (*yacad_statement_get_int_fn)(yacad_statement_t *this, int index);
typedef const char *(*yacad_statement_get_string_fn)(yacad_statement_t *this, int index);
typedef void (*yacad_statement_free_fn)(yacad_statement_t *this);

struct yacad_statement_s {
     yacad_statement_bind_int_fn bind_int;
     yacad_statement_bind_string_fn bind_string;
     yacad_statement_run_fn run;
     yacad_statement_get_rowid_fn get_rowid;
     yacad_statement_get_int_fn get_int;
     yacad_statement_get_string_fn get_string;
     yacad_statement_free_fn free;
};

yacad_database_t *yacad_database_new(logger_t log, const char *database_name);

#endif /* __YACAD_DATABASE_H__ */
