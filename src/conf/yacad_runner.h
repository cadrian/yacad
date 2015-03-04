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

typedef struct yacad_runner_s yacad_runner_t;

typedef void (*yacad_runner_free_fn)(yacad_runner_t *this);

struct yacad_runner_s {
     yacad_runner_free_fn free;
};

#endif /* __YACAD_RUNNER_H__ */
