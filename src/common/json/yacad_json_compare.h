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

#ifndef __YACAD_JSON_COMPARE_H__
#define __YACAD_JSON_COMPARE_H__

#include "yacad.h"

typedef struct yacad_json_compare_s yacad_json_compare_t;

typedef bool_t (*yacad_json_equal_fn)(yacad_json_compare_t *this, json_value_t *v1, json_value_t *v2);

struct yacad_json_compare_s {
     json_visitor_t fn;
     yacad_json_equal_fn equal;
};

yacad_json_compare_t *yacad_json_compare_new(logger_t log);

#endif /* __YACAD_JSON_COMPARE_H__ */
