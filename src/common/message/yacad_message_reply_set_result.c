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

#include "yacad_message_reply_set_result.h"
#include "yacad_message_visitor.h"
#include "common/json/yacad_json_finder.h"
#include "common/json/yacad_json_template.h"

typedef struct yacad_message_reply_set_result_impl_s {
     yacad_message_reply_set_result_t fn;
     yacad_runnerid_t *runnerid;
     json_value_t *serial;
} yacad_message_reply_set_result_impl_t;

static void accept(yacad_message_reply_set_result_impl_t *this, yacad_message_visitor_t *visitor) {
     visitor->visit_reply_set_result(visitor, I(this));
}

static char *serialize(yacad_message_reply_set_result_impl_t *this) {
     char *result = NULL;
     const char *runnerid = this->runnerid->serialize(this->runnerid);
     int n;

     n = snprintf("", 0, "{\"type\":\"reply_set_result\",\"runner\":%s}", runnerid) + 1;
     result = malloc(n);
     snprintf(result, n, "{\"type\":\"reply_set_result\",\"runner\":%s}", runnerid);

     return result;
}

static yacad_runnerid_t *get_runnerid(yacad_message_reply_set_result_impl_t *this) {
     return this->runnerid;
}

static void free_(yacad_message_reply_set_result_impl_t *this) {
     free(this);
}

static yacad_message_reply_set_result_t impl_fn = {
     .fn = {
          .accept = (yacad_message_accept_fn)accept,
          .serialize = (yacad_message_serialize_fn)serialize,
          .free = (yacad_message_free_fn)free_,
     },
     .get_runnerid = (yacad_message_reply_set_result_get_runnerid_fn)get_runnerid,
};

yacad_message_reply_set_result_t *yacad_message_reply_set_result_new(logger_t log, yacad_runnerid_t *runnerid) {
     yacad_message_reply_set_result_impl_t *result = malloc(sizeof(yacad_message_reply_set_result_impl_t));

     result->fn = impl_fn;
     result->runnerid = runnerid;
     result->serial = NULL;

     return I(result);
}

yacad_message_reply_set_result_t *yacad_message_reply_set_result_unserialize(logger_t log, json_value_t *jserial, cad_hash_t *env) {
     yacad_message_reply_set_result_impl_t *result = malloc(sizeof(yacad_message_reply_set_result_impl_t));
     yacad_json_finder_t *v = yacad_json_finder_new(log, json_type_object, "%s");
     yacad_json_template_t *t = yacad_json_template_new(log, NULL);

     result->fn = impl_fn;
     result->serial = t->resolve(t, jserial);
     v->visit(v, result->serial, "runner");
     result->runnerid = yacad_runnerid_new(log, v->get_value(v));

     I(t)->free(I(t));
     I(v)->free(I(v));
     return I(result);
}
