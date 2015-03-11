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

#include <sys/time.h>

#include <json.h>

#include "yacad.h"
#include "conf/yacad_conf.h"
#include "cron/yacad_cron.h"
#include "scm/yacad_scm.h"

typedef struct yacad_project_s yacad_project_t;

typedef const char *(*yacad_project_get_name_fn)(yacad_project_t *this);
typedef struct timeval (*yacad_project_next_check_fn)(yacad_project_t *this);
typedef bool_t (*yacad_project_check_fn)(yacad_project_t *this);
typedef json_object_t *(*yacad_get_runner_criteria_fn)(yacad_project_t *this);
typedef void (*yacad_project_free_fn)(yacad_project_t *this);

struct yacad_project_s {
     yacad_project_get_name_fn get_name;
     yacad_project_next_check_fn next_check;
     yacad_project_check_fn check;
     yacad_get_runner_criteria_fn get_runner_criteria;
     yacad_project_free_fn free;
};

yacad_project_t *yacad_project_new(yacad_conf_t *conf, const char *name, yacad_scm_t *scm, yacad_cron_t *cron, json_object_t *runner_criteria);

#endif /* __YACAD_PROJECT_H__ */
