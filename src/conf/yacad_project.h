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

#include "yacad.h"

typedef struct yacad_project_s yacad_project_t;

typedef void (*yacad_project_free_fn)(yacad_project_t *this);

struct yacad_project_s {
     yacad_project_free_fn free;
};

#endif /* __YACAD_PROJECT_H__ */
