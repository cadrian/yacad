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

#ifndef __YACAD_DATABASE_SQLITE3_H__
#define __YACAD_DATABASE_SQLITE3_H__

#include <sqlite3.h>

#include "yacad_database.h"

typedef struct sqlite3_fn_s {
      typeof(sqlite3_errmsg           )   *errmsg           ;
      typeof(sqlite3_bind_int64       )   *bind_int64       ;
      typeof(sqlite3_bind_text64      )   *bind_text64      ;
      typeof(sqlite3_step             )   *step             ;
      typeof(sqlite3_last_insert_rowid)   *last_insert_rowid;
      typeof(sqlite3_column_int64     )   *column_int64     ;
      typeof(sqlite3_column_text      )   *column_text      ;
      typeof(sqlite3_finalize         )   *finalize         ;
      typeof(sqlite3_prepare_v2       )   *prepare          ;
      typeof(sqlite3_close            )   *close            ;
      typeof(sqlite3_initialize       )   *initialize       ;
      typeof(sqlite3_open_v2          )   *open             ;
      typeof(sqlite3_exec             )   *exec             ;
} sqlite3_fn_t;

void yacad_database_set_sqlite_fn(sqlite3_fn_t fn);

#endif /* __YACAD_DATABASE_SQLITE3_H__ */
