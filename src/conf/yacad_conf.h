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

#include <cad_shared.h>
#include <cad_hash.h>

#include "yacad.h"
#include "tasklist/yacad_task.h"

/**
 * The log levels
 */
typedef enum {
     /**
      * Warnings (default level)
      */
     warn=0,
     /**
      * Informations on how ExP runs
      */
     info,
     /**
      * Developer details, usually not useful
      */
     debug,
     /**
      * Developper useless details
      */
     trace,
} level_t;

/**
 * A logger is a `printf`-like function to call
 *
 * @param[in] level the logging level
 * @param[in] format the message format
 * @param[in] ... the message arguments
 *
 * @return the length of the message after expansion; 0 if the log is not emitted because of the level
 */
typedef int (*logger_t) (level_t level, char *format, ...) __PRINTF__;

typedef struct yacad_conf_s yacad_conf_t;

typedef const char *(*yacad_conf_get_database_name_fn)(yacad_conf_t *this);
typedef yacad_task_t *(*yacad_conf_next_task_fn)(yacad_conf_t *this);
typedef cad_hash_t *(*yacad_conf_get_projects_fn)(yacad_conf_t *this);
typedef cad_hash_t *(*yacad_conf_get_runners_fn)(yacad_conf_t *this);
typedef int (*yacad_conf_generation_fn)(yacad_conf_t *this);
typedef void (*yacad_conf_free_fn)(yacad_conf_t *this);

struct yacad_conf_s {
     logger_t log;
     yacad_conf_get_database_name_fn get_database_name;
     yacad_conf_next_task_fn next_task;
     yacad_conf_get_projects_fn get_projects;
     yacad_conf_get_runners_fn get_runners;
     yacad_conf_generation_fn generation;
     yacad_conf_free_fn free;
};

yacad_conf_t *yacad_conf_new(void);

#endif /* __YACAD_CONF_H__ */
