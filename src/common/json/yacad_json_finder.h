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

#ifndef __YACAD_JSON_FINDER_H__
#define __YACAD_JSON_FINDER_H__

#include "yacad.h"

typedef enum {
     json_type_object,
     json_type_array,
     json_type_string,
     json_type_number,
     json_type_const,
} json_type_t;

typedef struct yacad_json_finder_s yacad_json_finder_t;

typedef json_object_t *(*yacad_json_finder_get_object_fn)(yacad_json_finder_t *this);
typedef json_array_t *(*yacad_json_finder_get_array_fn)(yacad_json_finder_t *this);
typedef json_string_t *(*yacad_json_finder_get_string_fn)(yacad_json_finder_t *this);
typedef json_number_t *(*yacad_json_finder_get_number_fn)(yacad_json_finder_t *this);
typedef json_const_t *(*yacad_json_finder_get_const_fn)(yacad_json_finder_t *this);
typedef void (*yacad_json_finder_vvisit_fn)(yacad_json_finder_t *this, json_value_t *value, va_list args);
typedef void (*yacad_json_finder_visit_fn)(yacad_json_finder_t *this, json_value_t *value, ...);

struct yacad_json_finder_s {
     json_visitor_t fn;
     yacad_json_finder_get_object_fn get_object;
     yacad_json_finder_get_array_fn get_array;
     yacad_json_finder_get_string_fn get_string;
     yacad_json_finder_get_number_fn get_number;
     yacad_json_finder_get_const_fn get_const;
     yacad_json_finder_vvisit_fn vvisit;
     yacad_json_finder_visit_fn visit;
};

yacad_json_finder_t *yacad_json_finder_new(logger_t log, json_type_t expected_type, const char *pathformat);

#endif /* __YACAD_JSON_FINDER_H__ */
