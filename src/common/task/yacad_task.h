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

#ifndef __YACAD_TASK_H__
#define __YACAD_TASK_H__

#include "yacad.h"
#include "common/runnerid/yacad_runnerid.h"

typedef struct yacad_task_s yacad_task_t;

typedef enum {
     task_new=0,
     task_running=1,
     task_done=-1,
     task_aborted=-2,
} yacad_task_status_t;

typedef unsigned long (*yacad_task_get_id_fn)(yacad_task_t *this);
typedef void (*yacad_task_set_id_fn)(yacad_task_t *this, unsigned long id);
typedef time_t (*yacad_task_get_timestamp_fn)(yacad_task_t *this);
typedef yacad_task_status_t (*yacad_task_get_status_fn)(yacad_task_t *this);
typedef void (*yacad_task_set_status_fn)(yacad_task_t *this, yacad_task_status_t status);
typedef json_value_t *(*yacad_task_get_source_fn)(yacad_task_t *this);
typedef json_value_t *(*yacad_task_get_run_fn)(yacad_task_t *this);
typedef yacad_runnerid_t *(*yacad_task_get_runnerid_fn)(yacad_task_t *this);
typedef const char *(*yacad_task_get_project_name_fn)(yacad_task_t *this);
typedef int (*yacad_task_get_taskindex_fn)(yacad_task_t *this);
typedef void (*yacad_task_set_taskindex_fn)(yacad_task_t *this, int index);
typedef cad_hash_t *(*yacad_task_get_env_fn)(yacad_task_t *this);
typedef char *(*yacad_task_serialize_fn)(yacad_task_t *this);
typedef bool_t (*yacad_task_same_as_fn)(yacad_task_t *this, yacad_task_t *other);
typedef void (*yacad_task_free_fn)(yacad_task_t *this);

struct yacad_task_s {
     yacad_task_get_id_fn get_id;
     yacad_task_set_id_fn set_id;
     yacad_task_get_timestamp_fn get_timestamp;
     yacad_task_get_status_fn get_status;
     yacad_task_set_status_fn set_status;
     yacad_task_get_source_fn get_source;
     yacad_task_get_run_fn get_run;
     yacad_task_get_runnerid_fn get_runnerid;
     yacad_task_get_project_name_fn get_project_name;
     yacad_task_get_taskindex_fn get_taskindex;
     yacad_task_set_taskindex_fn set_taskindex;
     yacad_task_get_env_fn get_env;
     yacad_task_serialize_fn serialize;
     yacad_task_same_as_fn same_as;
     yacad_task_free_fn free;
};

yacad_task_t *yacad_task_unserialize(logger_t log, char *serial);
yacad_task_t *yacad_task_new(logger_t log, json_value_t *task, cad_hash_t *env, const char *project_name, int taskindex);

#endif /* __YACAD_TASK_H__ */
