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

#include "yacad_json_compare.h"

typedef struct {
     json_visitor_t fn;
     logger_t log;
     union {
          json_value_t *value;
          json_object_t *object;
          json_array_t *array;
          json_string_t *string;
          json_number_t *number;
          json_const_t *constant;
     } data;
} yacad_json_compare_v2_t;

typedef struct {
     yacad_json_compare_t fn;
     yacad_json_compare_v2_t other;
     bool_t result;
} yacad_json_compare_impl_t;

static void free_visitor(yacad_json_compare_impl_t *this) {
     free(this);
}

static void v2_object(yacad_json_compare_v2_t *this, json_object_t *visited) {
     this->data.object = visited;
}

static void v2_array(yacad_json_compare_v2_t *this, json_array_t *visited) {
     this->data.array = visited;
}

static void v2_string(yacad_json_compare_v2_t *this, json_string_t *visited) {
     this->data.string = visited;
}

static void v2_number(yacad_json_compare_v2_t *this, json_number_t *visited) {
     this->data.number = visited;
}

static void v2_const(yacad_json_compare_v2_t *this, json_const_t *visited) {
     this->data.constant = visited;
}

static json_visitor_t v2_fn = {
     .free = (json_visit_free_fn)free_visitor,
     .visit_object = (json_visit_object_fn)v2_object,
     .visit_array = (json_visit_array_fn)v2_array,
     .visit_string = (json_visit_string_fn)v2_string,
     .visit_number = (json_visit_number_fn)v2_number,
     .visit_const = (json_visit_const_fn)v2_const,
};

static void compare_object(yacad_json_compare_impl_t *this, json_object_t *v1, json_object_t *v2) {
     unsigned int i, n = v1->count(v1);
     char **keys;
     json_value_t *x1, *x2;
     if (n != v2->count(v2)) {
          this->result = false;
     } else {
          this->result = true;
          keys = malloc(n);
          v1->keys(v1, (const char **)keys);
          for (i = 0; this->result && i < n; i++) {
               x1 = v1->get(v1, keys[i]);
               x2 = v2->get(v2, keys[i]);
               if (x1 == NULL) {
                    this->result = x2 == NULL;
               } else if (x2 == NULL) {
                    this->result = false;
               } else {
                    this->other.data.object = v2;
                    v1->accept(v1, I(I(this)));
               }
          }
          free(keys);
     }
}

static void compare_array(yacad_json_compare_impl_t *this, json_array_t *v1, json_array_t *v2) {
     unsigned int i, n = v1->count(v1);
     json_value_t *x1, *x2;
     if (n != v2->count(v2)) {
          this->result = false;
     } else {
          this->result = true;
          for (i = 0; this->result && i < n; i++) {
               x1 = v1->get(v1, i);
               x2 = v2->get(v2, i);
               if (x1 == NULL) {
                    this->result = x2 == NULL;
               } else if (x2 == NULL) {
                    this->result = false;
               } else {
                    this->other.data.array = v2;
                    v1->accept(v1, I(I(this)));
               }
          }
     }
}

static void compare_string(yacad_json_compare_impl_t *this, json_string_t *v1, json_string_t *v2) {
     int i, n = v1->count(v1);
     if (n != v2->count(v2)) {
          this->result = false;
     } else {
          this->result = true;
          for (i = 0; this->result && i < n; i++) {
               this->result = v1->get(v1, i) == v2->get(v2, i);
          }
     }
}

static void compare_number(yacad_json_compare_impl_t *this, json_number_t *v1, json_number_t *v2) {
     if (v1->is_int(v1)) {
          this->result = v2->is_int(v2) && v1->to_int(v1) == v2->to_int(v2);
     } else {
          this->result = v1->to_double(v1) == v2->to_double(v2);
     }
}

static void compare_const(yacad_json_compare_impl_t *this, json_const_t *v1, json_const_t *v2) {
     this->result = v1->value(v1) == v2->value(v2);
}

#define DEFUN_VISIT(type, field) static void visit_##type(yacad_json_compare_impl_t *this, json_##type##_t *visited) { \
          json_value_t *old_v2 = this->other.data.value;                \
          this->other.data.value = NULL;                                \
          old_v2->accept(old_v2, I(&(this->other)));                    \
          if (this->other.data.field != NULL) {                         \
               compare_##type(this, visited, this->other.data.field);   \
          }                                                             \
          this->other.data.value = old_v2;                              \
     }

DEFUN_VISIT(object, object)
DEFUN_VISIT(array, array)
DEFUN_VISIT(string, string)
DEFUN_VISIT(number, number)
DEFUN_VISIT(const, constant)

static bool_t equal(yacad_json_compare_impl_t *this, json_value_t *v1, json_value_t *v2) {
     this->other.data.value = v2;
     this->result = false;
     v1->accept(v1, I(I(this)));
     return this->result;
}

static yacad_json_compare_t impl_fn = {
     .fn = {
          .free = (json_visit_free_fn)free_visitor,
          .visit_object = (json_visit_object_fn)visit_object,
          .visit_array = (json_visit_array_fn)visit_array,
          .visit_string = (json_visit_string_fn)visit_string,
          .visit_number = (json_visit_number_fn)visit_number,
          .visit_const = (json_visit_const_fn)visit_const,
     },
     .equal = (yacad_json_equal_fn)equal,
};

yacad_json_compare_t *yacad_json_compare_new(logger_t log) {
     yacad_json_compare_impl_t *result = malloc(sizeof(yacad_json_compare_impl_t));
     result->fn = impl_fn;
     result->other.fn = v2_fn;
     result->other.data.value = NULL;
     result->result = false;
     return I(result);
}
