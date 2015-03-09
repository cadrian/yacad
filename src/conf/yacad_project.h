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

#include "yacad.h"
#include "yacad_conf.h"

typedef struct yacad_project_s yacad_project_t;

typedef void (*on_success_action)(yacad_project_t *this, void *data);

typedef struct timeval (*yacad_project_next_check_fn)(yacad_project_t *this);
typedef bool_t (*yacad_project_check_fn)(yacad_project_t *this, on_success_action on_success, void *data);
typedef void (*yacad_project_free_fn)(yacad_project_t *this);

struct yacad_project_s {
     yacad_project_next_check_fn next_check;
     yacad_project_check_fn check;
     yacad_project_free_fn free;
};

yacad_project_t *yacad_project_new(yacad_conf_t *conf, const char *name, const char *scm,
                                   const char *root_path, const char *upstream_url, const char *cron);

#endif /* __YACAD_PROJECT_H__ */
