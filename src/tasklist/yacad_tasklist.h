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
#include "yacad_task.h"
#include "conf/yacad_conf.h"

typedef struct yacad_tasklist_s yacad_tasklist_t;

typedef void (*yacad_tasklist_add_fn)(yacad_tasklist_t *this, yacad_task_t *task);
typedef void (*yacad_tasklist_free_fn)(yacad_tasklist_t *this);
typedef yacad_task_t *(*yacad_tasklist_get_fn)(yacad_tasklist_t *this, const char *runner_name);

struct yacad_tasklist_s {
     yacad_tasklist_add_fn add;
     yacad_tasklist_get_fn get;
     yacad_tasklist_free_fn free;
};

yacad_tasklist_t *yacad_tasklist_new(yacad_conf_t *conf);

#endif /* __YACAD_TASKLIST_H__ */
