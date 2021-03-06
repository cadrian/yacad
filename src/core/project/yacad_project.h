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

#ifndef __YACAD_PROJECT_H__
#define __YACAD_PROJECT_H__

#include <json.h>

#include "yacad.h"
#include "common/cron/yacad_cron.h"
#include "common/scm/yacad_scm.h"
#include "common/task/yacad_task.h"

typedef struct yacad_project_s yacad_project_t;

typedef const char *(*yacad_project_get_name_fn)(yacad_project_t *this);
typedef struct timeval (*yacad_project_next_check_fn)(yacad_project_t *this);
typedef yacad_task_t *(*yacad_project_check_fn)(yacad_project_t *this);
typedef yacad_task_t *(*yacad_project_next_task_fn)(yacad_project_t *this, yacad_task_t *previous);
typedef yacad_scm_t *(*yacad_project_get_scm_fn)(yacad_project_t *this);
typedef void (*yacad_project_free_fn)(yacad_project_t *this);

struct yacad_project_s {
     yacad_project_get_name_fn get_name;
     yacad_project_next_check_fn next_check;
     yacad_project_check_fn check;
     yacad_project_next_task_fn next_task;
     yacad_project_get_scm_fn get_scm;
     yacad_project_free_fn free;
};

yacad_project_t *yacad_project_new(logger_t log, const char *name, const char *root_path, yacad_cron_t *cron, yacad_scm_t *scm, json_array_t *tasks);

#endif /* __YACAD_PROJECT_H__ */
