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

#include "yacad_message_query_get_task.h"
#include "yacad_message_visitor.h"
#include "common/json/yacad_json_finder.h"

typedef struct yacad_message_query_get_task_impl_s {
     yacad_message_query_get_task_t fn;
     yacad_runnerid_t *runnerid;
} yacad_message_query_get_task_impl_t;

static void accept(yacad_message_query_get_task_impl_t *this, yacad_message_visitor_t *visitor) {
     visitor->visit_query_get_task(visitor, I(this));
}

static char *serialize(yacad_message_query_get_task_impl_t *this) {
     char *result = NULL;
     const char *runnerid = this->runnerid->serialize(this->runnerid);
     int n = snprintf("", 0, "{\"type\":\"query_get_task\",\"runner\":%s}", runnerid) + 1;
     result = malloc(n);
     snprintf(result, n, "{\"type\":\"query_get_task\",\"runner\":%s}", runnerid);
     return result;
}

static void free_(yacad_message_query_get_task_impl_t *this) {
     this->runnerid->free(this->runnerid);
     free(this);
}

static yacad_runnerid_t *get_runnerid(yacad_message_query_get_task_impl_t *this) {
     return this->runnerid;
}

static yacad_message_query_get_task_t impl_fn = {
     .fn = {
          .accept = (yacad_message_accept_fn)accept,
          .serialize = (yacad_message_serialize_fn)serialize,
          .free = (yacad_message_free_fn)free_,
     },
     .get_runnerid = (yacad_message_query_get_task_get_runnerid_fn)get_runnerid,
};

yacad_message_query_get_task_t *yacad_message_query_get_task_new(logger_t log, yacad_runnerid_t *runnerid) {
     yacad_message_query_get_task_impl_t *result = malloc(sizeof(yacad_message_query_get_task_impl_t));
     result->fn = impl_fn;
     result->runnerid = yacad_runnerid_unserialize(log, (char*)runnerid->serialize(runnerid));
     return I(result);
}

yacad_message_query_get_task_t *yacad_message_query_get_task_unserialize(logger_t log, json_value_t *jserial, cad_hash_t *env) {
     yacad_message_query_get_task_impl_t *result = malloc(sizeof(yacad_message_query_get_task_impl_t));
     yacad_json_finder_t *v = yacad_json_finder_new(log, json_type_object, "runner");

     v->visit(v, jserial);
     result->fn = impl_fn;
     result->runnerid = yacad_runnerid_new(log, v->get_value(v));

     I(v)->free(I(v));
     return I(result);
}
