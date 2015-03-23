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

#include "yacad_message_reply_get_task.h"
#include "yacad_message_visitor.h"
#include "common/json/yacad_json_finder.h"
#include "common/json/yacad_json_template.h"

typedef struct yacad_message_reply_get_task_impl_s {
     yacad_message_reply_get_task_t fn;
     yacad_runnerid_t *runnerid;
     yacad_scm_t *scm;
     yacad_task_t *task;
     json_value_t *serial;
} yacad_message_reply_get_task_impl_t;

static void accept(yacad_message_reply_get_task_impl_t *this, yacad_message_visitor_t *visitor) {
     visitor->visit_reply_get_task(visitor, I(this));
}

static char *serialize(yacad_message_reply_get_task_impl_t *this) {
     char *result = NULL;
     const char *runnerid = this->runnerid->serialize(this->runnerid);
     char *task = this->task->serialize(this->task);
     json_value_t *scm_desc = this->scm->get_desc(this->scm);
     char *scm;
     json_output_stream_t *scm_out = new_json_output_stream_from_string(&scm, stdlib_memory);
     json_visitor_t *wscm = json_write_to(scm_out, stdlib_memory, 0);
     int n;

     scm_desc->accept(scm_desc, wscm);

     n = snprintf("", 0, "{\"type\":\"reply_get_task\",\"runner\":%s,\"task\":%s,\"scm\":%s}", runnerid, task, scm) + 1;
     result = malloc(n);
     snprintf(result, n, "{\"type\":\"reply_get_task\",\"runner\":%s,\"task\":%s,\"scm\":%s}", runnerid, task, scm);

     wscm->free(wscm);
     scm_out->free(scm_out);
     free(task);

     return result;
}

static void free_(yacad_message_reply_get_task_impl_t *this) {
     if (this->serial != NULL) {
          this->runnerid->free(this->runnerid);
          this->scm->free(this->scm);
          this->task->free(this->task);
          this->serial->accept(this->serial, json_kill());
     }
     free(this);
}

static yacad_runnerid_t *get_runnerid(yacad_message_reply_get_task_impl_t *this) {
     return this->runnerid;
}

static yacad_scm_t *get_scm(yacad_message_reply_get_task_impl_t *this) {
     return this->scm;
}

static yacad_task_t *get_task(yacad_message_reply_get_task_impl_t *this) {
     return this->task;
}

static yacad_message_reply_get_task_t impl_fn = {
     .fn = {
          .accept = (yacad_message_accept_fn)accept,
          .serialize = (yacad_message_serialize_fn)serialize,
          .free = (yacad_message_free_fn)free_,
     },
     .get_runnerid = (yacad_message_reply_get_task_get_runnerid_fn)get_runnerid,
     .get_scm = (yacad_message_reply_get_task_get_scm_fn)get_scm,
     .get_task = (yacad_message_reply_get_task_get_task_fn)get_task,
};

yacad_message_reply_get_task_t *yacad_message_reply_get_task_new(logger_t log, yacad_runnerid_t *runnerid, yacad_scm_t *scm, yacad_task_t *task) {
     yacad_message_reply_get_task_impl_t *result = malloc(sizeof(yacad_message_reply_get_task_impl_t));

     result->fn = impl_fn;
     result->runnerid = runnerid;
     result->scm = scm;
     result->task = task;
     result->serial = NULL;

     return I(result);
}

yacad_message_reply_get_task_t *yacad_message_reply_get_task_unserialize(logger_t log, json_value_t *jserial, cad_hash_t *env) {
     yacad_message_reply_get_task_impl_t *result = malloc(sizeof(yacad_message_reply_get_task_impl_t));
     yacad_json_finder_t *v = yacad_json_finder_new(log, json_type_object, "%s");
     yacad_json_template_t *t = yacad_json_template_new(log, NULL);
     json_value_t *task;
     char *root_path = env->get(env, "root_path");
     char *stask;
     json_output_stream_t *out_task = new_json_output_stream_from_string(&stask, stdlib_memory);
     json_visitor_t *wtask = json_write_to(out_task, stdlib_memory, 0);

     result->fn = impl_fn;
     result->serial = t->resolve(t, jserial);
     v->visit(v, result->serial, "runner");
     result->runnerid = yacad_runnerid_new(log, v->get_value(v));
     v->visit(v, result->serial, "scm");
     result->scm = yacad_scm_new(log, v->get_value(v), root_path);
     v->visit(v, result->serial, "task");
     task = v->get_value(v);

     task->accept(task, wtask);

     result->task = yacad_task_unserialize(log, stask);

     wtask->free(wtask);
     out_task->free(out_task);
     free(stask);

     I(t)->free(I(t));
     I(v)->free(I(v));
     return I(result);
}
