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
     const char *result = this->runnerid->serialize(this->runnerid);
     return strdup(result);
}

static void free_(yacad_message_reply_get_task_impl_t *this) {
     if (this->serial != NULL) {
          this->runnerid->free(this->runnerid);
          this->scm->free(this->scm);
          this->task->free(this->task);
          this->serial->free(this->serial);
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
     json_value_t *task_run, *task_source;
     char *root_path = env->get(env, "root_path");

     v->visit(v, jserial, "runner");
     result->fn = impl_fn;
     result->serial = t->resolve(t, jserial);
     result->runnerid = yacad_runnerid_new(log, v->get_value(v));
     v->visit(v, result->serial, "scm");
     result->scm = yacad_scm_new(log, v->get_value(v), root_path);
     v->visit(v, result->serial, "task/run");
     task_run = v->get_value(v);
     v->visit(v, result->serial, "task/source");
     task_source = v->get_value(v);
     result->task = yacad_task_restore(log, 0UL, (time_t)0, task_new, task_run, task_source, result->runnerid);

     I(t)->free(I(t));
     I(v)->free(I(v));
     return I(result);
}
