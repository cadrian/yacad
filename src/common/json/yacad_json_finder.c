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

#include "yacad_json_finder.h"

typedef struct {
     yacad_json_finder_t fn;
     logger_t log;
     char *current_path;
     json_type_t expected_type;
     bool_t found;
     union {
          json_value_t *value;
          json_object_t *object;
          json_array_t *array;
          json_string_t *string;
          json_number_t *number;
          json_const_t *constant;
     } value;
     const char *pathformat;
} yacad_json_finder_impl_t;

static json_value_t *get_value(yacad_json_finder_impl_t *this) {
     return this->found ? this->value.value : NULL;
}

static json_object_t *get_object(yacad_json_finder_impl_t *this) {
     return this->found ? this->value.object : NULL;
}

static json_array_t *get_array(yacad_json_finder_impl_t *this) {
     return this->found ? this->value.array : NULL;
}

static json_string_t *get_string(yacad_json_finder_impl_t *this) {
     return this->found ? this->value.string : NULL;
}

static json_number_t *get_number(yacad_json_finder_impl_t *this) {
     return this->found ? this->value.number : NULL;
}

static json_const_t *get_const(yacad_json_finder_impl_t *this) {
     return this->found ? this->value.constant : NULL;
}

static void free_visitor(yacad_json_finder_impl_t *this) {
     free(this);
}

static void visit_object(yacad_json_finder_impl_t *this, json_object_t *visited) {
     char *key = this->current_path;
     char *next_path;
     json_value_t *value;
     this->value.object = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_object);
     } else {
          next_path = strchrnul(key, '/');
          this->current_path = next_path + (*next_path ? 1 : 0);
          *next_path = '\0';
          value = visited->get(visited, key);
          if (value != NULL) {
               value->accept(value, I(I(this)));
          }
     }
}

static void visit_array(yacad_json_finder_impl_t *this, json_array_t *visited) {
     char *key = this->current_path;
     char *next_path;
     json_value_t *value;
     int index;
     this->value.array = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_array);
     } else {
          next_path = strchrnul(key, '/');
          this->current_path = next_path + (*next_path ? 1 : 0);
          *next_path = '\0';
          index = atoi(key);
          value = visited->get(visited, index);
          if (value != NULL) {
               value->accept(value, I(I(this)));
          }
     }
}

static void visit_string(yacad_json_finder_impl_t *this, json_string_t *visited) {
     this->value.string = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_string);
     }
}

static void visit_number(yacad_json_finder_impl_t *this, json_number_t *visited) {
     this->value.number = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_number);
     }
}

static void visit_const(yacad_json_finder_impl_t *this, json_const_t *visited) {
     this->value.constant = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_const);
     }
}

static void vvisit(yacad_json_finder_impl_t *this, json_value_t *value, va_list vargs) {
     va_list args;
     int n;
     char *path;

     va_copy(args, vargs);
     n = vsnprintf("", 0, this->pathformat, args) + 1;
     va_end(args);

     path = malloc(n);

     va_copy(args, vargs);
     vsnprintf(path, n, this->pathformat, args);
     va_end(args);

     this->current_path = path;
     value->accept(value, I(I(this)));
     free(path);
}

static void visit(yacad_json_finder_impl_t *this, json_value_t *value, ...) {
     va_list args;
     va_start(args, value);
     vvisit(this, value, args);
     va_end(args);
}

static yacad_json_finder_t impl_fn = {
     .fn = {
          .free = (json_visit_free_fn)free_visitor,
          .visit_object = (json_visit_object_fn)visit_object,
          .visit_array = (json_visit_array_fn)visit_array,
          .visit_string = (json_visit_string_fn)visit_string,
          .visit_number = (json_visit_number_fn)visit_number,
          .visit_const = (json_visit_const_fn)visit_const,
     },
     .get_value = (yacad_json_finder_get_value_fn)get_value,
     .get_object = (yacad_json_finder_get_object_fn)get_object,
     .get_array = (yacad_json_finder_get_array_fn)get_array,
     .get_string = (yacad_json_finder_get_string_fn)get_string,
     .get_number = (yacad_json_finder_get_number_fn)get_number,
     .get_const = (yacad_json_finder_get_const_fn)get_const,
     .vvisit = (yacad_json_finder_vvisit_fn)vvisit,
     .visit = (yacad_json_finder_visit_fn)visit,
};

yacad_json_finder_t *yacad_json_finder_new(logger_t log, json_type_t expected_type, const char *pathformat) {
     yacad_json_finder_impl_t *result = malloc(sizeof(yacad_json_finder_impl_t));
     result->fn = impl_fn;
     result->log = log;
     result->found = false;
     result->expected_type = expected_type;
     result->pathformat = pathformat;
     result->value.object = NULL;
     return I(result);
}
