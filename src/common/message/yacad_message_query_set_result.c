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
#include "yacad_message_visitor.h"
#include "common/json/yacad_json_finder.h"
#include "common/json/yacad_json_template.h"

typedef struct yacad_message_query_set_result_impl_s {
     yacad_message_query_set_result_t fn;
     yacad_runnerid_t *runnerid;
     yacad_task_t *task;
     bool_t success;
     json_value_t *serial;
} yacad_message_query_set_result_impl_t;

static void accept(yacad_message_query_set_result_impl_t *this, yacad_message_visitor_t *visitor) {
     visitor->visit_query_set_result(visitor, I(this));
}

static yacad_runnerid_t *get_runnerid(yacad_message_query_set_result_impl_t *this) {
     return this->runnerid;
}

static yacad_task_t *get_task(yacad_message_query_set_result_impl_t *this) {
     return this->task;
}

static bool_t is_successful(yacad_message_query_set_result_impl_t *this) {
     return this->success;
}

static char *serialize(yacad_message_query_set_result_impl_t *this) {
     char *result = NULL;
     const char *runnerid = this->runnerid->serialize(this->runnerid);
     char *task = this->task->serialize(this->task);
     const char * success = this->success ? "true" : "false";
     int n;

     n = snprintf("", 0, "{\"type\":\"query_set_result\",\"runner\":%s,\"task\":%s,\"success\":%s", runnerid, task, success) + 1;
     result = malloc(n);
     snprintf(result, n, "{\"type\":\"query_set_result\",\"runner\":%s,\"task\":%s,\"success\":%s", runnerid, task, success);

     free(task);

     return result;
}

static void free_(yacad_message_query_set_result_impl_t *this) {
     if (this->serial != NULL) {
          this->runnerid->free(this->runnerid);
          this->task->free(this->task);
          this->serial->accept(this->serial, json_kill());
     }
     free(this);
}

static yacad_message_query_set_result_t impl_fn = {
     .fn = {
          .accept = (yacad_message_accept_fn)accept,
          .serialize = (yacad_message_serialize_fn)serialize,
          .free = (yacad_message_free_fn)free_,
     },
     .get_runnerid = (yacad_message_query_set_result_get_runnerid_fn)get_runnerid,
     .get_task = (yacad_message_query_set_result_get_task_fn)get_task,
     .is_successful = (yacad_message_query_set_result_is_successful_fn)is_successful,
};

yacad_message_query_set_result_t *yacad_message_query_set_result_new(logger_t log, yacad_runnerid_t *runnerid, yacad_task_t *task, bool_t success) {
     yacad_message_query_set_result_impl_t *result = malloc(sizeof(yacad_message_query_set_result_impl_t));

     result->fn = impl_fn;
     result->runnerid = runnerid;
     result->task = task;
     result->success = success;
     result->serial = NULL;

     return I(result);
}

yacad_message_query_set_result_t *yacad_message_query_set_result_unserialize(logger_t log, json_value_t *jserial, cad_hash_t *env) {
     yacad_message_query_set_result_impl_t *result = malloc(sizeof(yacad_message_query_set_result_impl_t));
     yacad_json_finder_t *v = yacad_json_finder_new(log, json_type_object, "%s");
     yacad_json_template_t *t = yacad_json_template_new(log, NULL);
     json_value_t *task;
     char *stask;
     json_output_stream_t *out_task = new_json_output_stream_from_string(&stask, stdlib_memory);
     json_visitor_t *wtask = json_write_to(out_task, stdlib_memory, 0);
     yacad_json_finder_t *b = yacad_json_finder_new(log, json_type_const, "%s");
     json_const_t *jsuccess;

     result->fn = impl_fn;

     result->serial = t->resolve(t, jserial);
     v->visit(v, result->serial, "runner");
     result->runnerid = yacad_runnerid_new(log, v->get_value(v));
     v->visit(v, result->serial, "task");
     task = v->get_value(v);

     task->accept(task, wtask);

     result->task = yacad_task_unserialize(log, stask);

     b->visit(b, result->serial, "success");
     jsuccess = b->get_const(b);
     result->success = jsuccess->value(jsuccess) == json_true;

     wtask->free(wtask);
     out_task->free(out_task);
     free(stask);

     I(b)->free(I(b));
     I(t)->free(I(t));
     I(v)->free(I(v));
     return I(result);
}
