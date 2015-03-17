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

#ifndef __YACAD_RUNNER_H__
#define __YACAD_RUNNER_H__

#include "yacad.h"

/**
 * The core-side representation of a runner. It manages its connexion
 * to the remote runner (ping, load discovery, task and results
 * transfer).
 */

typedef struct yacad_runner_s yacad_runner_t;

typedef const char *(*yacad_runner_get_name_fn)(yacad_runner_t *this);
typedef const char *(*yacad_runner_get_arch_fn)(yacad_runner_t *this);
typedef void (*yacad_runner_free_fn)(yacad_runner_t *this);

struct yacad_runner_s {
     yacad_runner_get_name_fn get_name;
     yacad_runner_get_arch_fn get_arch;
     yacad_runner_free_fn free;
};

yacad_runner_t *yacad_runner_new(json_object_t *desc);
yacad_runner_t *yacad_runner_select(const yacad_runner_t **runners, size_t szrunners, json_object_t *criteria);

#endif /* __YACAD_RUNNER_H__ */
