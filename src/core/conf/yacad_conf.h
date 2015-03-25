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

#ifndef __YACAD_CONF_H__
#define __YACAD_CONF_H__

#include "yacad.h"

typedef struct yacad_conf_s yacad_conf_t;

typedef const char *(*yacad_conf_get_database_name_fn)(yacad_conf_t *this);
typedef const char *(*yacad_conf_get_endpoint_name_fn)(yacad_conf_t *this);
typedef const char *(*yacad_conf_get_events_name_fn)(yacad_conf_t *this);
typedef cad_hash_t *(*yacad_conf_get_projects_fn)(yacad_conf_t *this);
typedef cad_hash_t *(*yacad_conf_get_runners_fn)(yacad_conf_t *this);
typedef int (*yacad_conf_generation_fn)(yacad_conf_t *this);
typedef void (*yacad_conf_free_fn)(yacad_conf_t *this);

struct yacad_conf_s {
     logger_t log;
     yacad_conf_get_database_name_fn get_database_name;
     yacad_conf_get_endpoint_name_fn get_endpoint_name;
     yacad_conf_get_events_name_fn get_events_name;
     yacad_conf_get_projects_fn get_projects;
     yacad_conf_get_runners_fn get_runners;
     yacad_conf_generation_fn generation;
     yacad_conf_free_fn free;
};

yacad_conf_t *yacad_conf_new(void);

#endif /* __YACAD_CONF_H__ */
