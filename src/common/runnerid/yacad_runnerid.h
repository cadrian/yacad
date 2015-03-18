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

#ifndef __YACAD_RUNNERID_H__
#define __YACAD_RUNNERID_H__

#include "yacad.h"

/**
 * The identification of a runner.
 */

typedef struct yacad_runnerid_s yacad_runnerid_t;

typedef const char *(*yacad_runnerid_get_name_fn)(yacad_runnerid_t *this);
typedef const char *(*yacad_runnerid_get_arch_fn)(yacad_runnerid_t *this);
typedef void (*yacad_runnerid_free_fn)(yacad_runnerid_t *this);

struct yacad_runnerid_s {
     yacad_runnerid_get_name_fn get_name;
     yacad_runnerid_get_arch_fn get_arch;
     yacad_runnerid_free_fn free;
};

yacad_runnerid_t *yacad_runnerid_new(logger_t log, json_value_t *desc);

#endif /* __YACAD_RUNNERID_H__ */
