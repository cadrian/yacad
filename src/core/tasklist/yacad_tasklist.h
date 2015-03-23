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

#ifndef __YACAD_TASKLIST_H__
#define __YACAD_TASKLIST_H__

#include "yacad.h"
#include "common/runnerid/yacad_runnerid.h"
#include "common/task/yacad_task.h"

typedef struct yacad_tasklist_s yacad_tasklist_t;

typedef void (*yacad_tasklist_add_fn)(yacad_tasklist_t *this, yacad_task_t *task);
typedef void (*yacad_tasklist_free_fn)(yacad_tasklist_t *this);
typedef yacad_task_t *(*yacad_tasklist_get_fn)(yacad_tasklist_t *this, yacad_runnerid_t *runnerid);
typedef void (*yacad_tasklist_set_task_aborted_fn)(yacad_tasklist_t *this, yacad_task_t *task);
typedef void (*yacad_tasklist_set_task_done_fn)(yacad_tasklist_t *this, yacad_task_t *task);

struct yacad_tasklist_s {
     yacad_tasklist_add_fn add;
     yacad_tasklist_get_fn get;
     yacad_tasklist_set_task_aborted_fn set_task_aborted;
     yacad_tasklist_set_task_done_fn set_task_done;
     yacad_tasklist_free_fn free;
};

yacad_tasklist_t *yacad_tasklist_new(logger_t log, const char *root_path);

#endif /* __YACAD_TASKLIST_H__ */
