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

#include <sys/time.h>

#include "yacad.h"

typedef struct yacad_task_s yacad_task_t;

typedef int (*yacad_task_get_id_fn)(yacad_task_t *this);
typedef const char *(*yacad_task_get_project_name_fn)(yacad_task_t *this);
typedef const char *(*yacad_task_get_runner_name_fn)(yacad_task_t *this);
typedef const char *(*yacad_task_get_action_fn)(yacad_task_t *this);
typedef const char *(*yacad_task_serialize_fn)(yacad_task_t *this);
typedef void (*yacad_task_free_fn)(yacad_task_t *this);

struct yacad_task_s {
     yacad_task_get_id_fn get_id;
     yacad_task_get_project_name_fn get_project_name;
     yacad_task_get_runner_name_fn get_runner_name;
     yacad_task_get_action_fn get_action;
     yacad_task_serialize_fn serialize;
     yacad_task_free_fn free;
};

yacad_task_t *yacad_task_unserialize(const char *serial);
yacad_task_t *yacad_task_new(int id, const char *project_name, const char *runner_name, const char *action);

#endif /* __YACAD_TASK_H__ */
