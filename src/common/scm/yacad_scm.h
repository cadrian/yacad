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

#ifndef __YACAD_SCM_H__
#define __YACAD_SCM_H__

#include "yacad.h"
#include "common/task/yacad_task.h"

typedef struct yacad_scm_s yacad_scm_t;

typedef yacad_task_t *(*yacad_scm_check_fn)(yacad_scm_t *this);
typedef void (*yacad_scm_free_fn)(yacad_scm_t *this);

struct yacad_scm_s {
     yacad_scm_check_fn check;
     yacad_scm_free_fn free;
};

yacad_scm_t *yacad_scm_new(logger_t log, json_value_t *desc, const char *root_path);

#endif /* __YACAD_SCM_H__ */
