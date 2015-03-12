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

#include "yacad_json_template.h"

typedef struct {
     yacad_json_template_t fn;
     logger_t log;
     union {
          json_value_t *value;
          json_object_t *object;
          json_array_t *array;
          json_string_t *string;
          json_number_t *number;
          json_const_t *constant;
     } result;
     cad_hash_t *env;
} yacad_json_template_impl_t;


static void free_visitor(yacad_json_template_impl_t *this) {
     this->env->free(this->env);
     free(this);
}

static void visit_object(yacad_json_template_impl_t *this, json_object_t *visited) {
     json_object_t *result = json_new_object(stdlib_memory);
     unsigned int i, count = visited->count(visited);
     const char **keys = malloc(count * sizeof(char*));
     json_value_t *value;
     visited->keys(visited, keys);
     for (i = 0; i < count; i++) {
          value = visited->get(visited, keys[i]);
          if (value == NULL) {
               result->set(result, keys[i], NULL);
          } else {
               value->accept(value, I(I(this)));
               result->set(result, keys[i], this->result.value);
          }
     }
     visited->keys(visited, (const char **)keys);
     free(keys);
     this->result.object = result;
}

static void visit_array(yacad_json_template_impl_t *this, json_array_t *visited) {
     json_array_t *result = json_new_array(stdlib_memory);
     unsigned int count = visited->count(visited);
     int i;
     json_value_t *value;
     for (i = 0; i < count; i++) {
          value = visited->get(visited, i);
          if (value == NULL) {
               result->set(result, i, NULL);
          } else {
               value->accept(value, I(I(this)));
               result->set(result, i, this->result.value);
          }
     }
     this->result.array = result;
}

static char *template_getenv(yacad_json_template_impl_t *this, const char *key, size_t szkey) {
     char *k = strndupa(key, szkey);
     return this->env->get(this->env, k);
}

static void visit_string(yacad_json_template_impl_t *this, json_string_t *visited) {
     json_string_t *result = json_new_string(stdlib_memory);
     size_t u = 0, s = 0, e, i, n = visited->utf8(visited, "", 0) + 1;
     char *template = malloc(n);
     char *env;
     char c;
     visited->utf8(visited, template, n);
     for (i = 0; i < n; i++) {
          c = template[i];
          if (u != 0 || c < ' ' || c > '~') { // partial utf-8
               u = result->add_utf8(result, c);
          } else {
               switch (s) {
               case 0:
                    if (c == '$') {
                         s = 1;
                    } else {
                         u = result->add_utf8(result, c);
                    }
                    break;
               case 1:
                    if (c == '{') {
                         e = i + 1;
                         s = 2;
                    } else {
                         result->add_utf8(result, '$');
                         u = result->add_utf8(result, c);
                         s = 0;
                    }
                    break;
               case 2:
                    if (c == '}') {
                         env = template_getenv(this, template + e, i - e);
                         if (env != NULL) {
                              result->add_string(result, "%s", env);
                         } else {
                              this->log(warn, "Unknown env ${%.*s}\n", (int)(i - e), template + e);
                         }
                         s = 0;
                    }
                    break;
               }
          }
     }
     this->result.string = result;
}

static void visit_number(yacad_json_template_impl_t *this, json_number_t *visited) {
     this->result.number = visited;
}

static void visit_const(yacad_json_template_impl_t *this, json_const_t *visited) {
     this->result.constant = visited;
}

static json_value_t *resolve(yacad_json_template_impl_t *this, json_value_t *template) {
     json_value_t *result = NULL;
     if (template != NULL) {
          template->accept(template, I(I(this)));
          result = this->result.value;
     }
     return result;
}

static yacad_json_template_t impl_fn = {
     .fn = {
          .free = (json_visit_free_fn)free_visitor,
          .visit_object = (json_visit_object_fn)visit_object,
          .visit_array = (json_visit_array_fn)visit_array,
          .visit_string = (json_visit_string_fn)visit_string,
          .visit_number = (json_visit_number_fn)visit_number,
          .visit_const = (json_visit_const_fn)visit_const,
     },
     .resolve = (yacad_json_template_resolve_fn)resolve,
};

yacad_json_template_t *yacad_json_template_new(logger_t log, cad_hash_t *env) {
     yacad_json_template_impl_t *result = malloc(sizeof(yacad_json_template_impl_t));
     result->fn = impl_fn;
     result->result.value = NULL;
     result->env = env;
     return I(result);
}
