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

#include "yacad_message_query_set_result.h"

typedef struct yacad_message_query_set_result_impl_s {
     yacad_message_query_set_result_t fn;
} yacad_message_query_set_result_impl_t;

static void accept(yacad_message_query_set_result_impl_t *this, yacad_message_visitor_t *visitor) {
     visitor->visit_query_set_result(visitor, I(this));
}

static char *serialize(yacad_message_query_set_result_impl_t *this) {
     return strdup("");
}

static void free_(yacad_message_query_set_result_impl_t *this) {
     free(this);
}

static yacad_message_query_set_result_t impl_fn = {
     .fn = {
          .accept = (yacad_message_accept_fn)accept,
          .serialize = (yacad_message_serialize_fn)serialize,
          .free = (yacad_message_free_fn)free_,
     },
};

yacad_message_query_set_result_t *yacad_message_query_set_result_unserialize(logger_t log, json_value_t *jserial, cad_hash_t *env) {
     yacad_message_query_set_result_impl_t *result = malloc(sizeof(yacad_message_query_set_result_impl_t));
     result->fn = impl_fn;
     return I(result);
}
