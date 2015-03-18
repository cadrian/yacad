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

#include "yacad_task.h"
#include "common/json/yacad_json_template.h"
#include "common/json/yacad_json_compare.h"

typedef struct yacad_task_impl_s {
     yacad_task_t fn;
     logger_t log;
     unsigned long id;
     time_t timestamp;
     yacad_task_status_t status;
     json_value_t *action;
     yacad_scm_t *scm;
     yacad_runnerid_t *runnerid;
     yacad_object_t *run;
} yacad_task_impl_t;

static unsigned long get_id(yacad_task_impl_t *this) {
     return this->id;
}

static void set_id(yacad_task_impl_t *this, unsigned long id) {
     this->id = id;
}

static time_t get_timestamp(yacad_task_impl_t *this) {
     return this->timestamp;
}

static yacad_task_status_t get_status(yacad_task_impl_t *this) {
     return this->status;
}

static void set_status(yacad_task_impl_t *this, yacad_task_status_t status) {
     this->status = status;
}

static yacad_runnerid_t *get_runnerid(yacad_task_impl_t *this) {
     return this->runnerid;
}

static yacad_scm_t *get_scm(yacad_task_impl_t *this) {
     return this->scm;
}

static json_object_t *get_run(yacad_task_impl_t *this) {
     return this->run;
}

static bool_t same_as(yacad_task_impl_t *this, yacad_task_impl_t *other) {
     bool_t result = false;
     static yacad_json_compare_t *cmp = NULL;

     if (this->desc == NULL) {
          result = other->desc == NULL;
     } else if (other->desc != NULL) {
          if (cmp == NULL) {
               cmp = yacad_json_compare_new(this->log);
          }
          result = cmp->equal(cmp, this->desc, other->desc);
     }
     if (result)  {
          result = this->actionindex = other->actionindex;
          if (result) {
               result = this->runnerid->same_as(this->runnerid, other->runnerid);
          }
     }
     return result;
}

static const char *serialize(yacad_task_impl_t *this) {
     static size_t capacity = 0;
     static char *result = NULL;
     char *desc = NULL;
     json_output_stream_t *out = new_json_output_stream_from_string(&desc, stdlib_memory);
     json_visitor_t *v = json_write_to(out, stdlib_memory, json_compact);
     size_t n;

     this->desc->accept(this->desc, v);

     n = snprintf(result, capacity, "%s", desc) + 1;
     if (n > capacity) {
          if (capacity == 0) {
               capacity = 4096;
          }
          while (capacity < n) {
               capacity *= 2;
          }
          result = realloc(result, capacity);
          snprintf(result, capacity, "%s", desc);
     }

     v->free(v);
     out->free(out);
     free(desc);
     return result;
}

static void free_(yacad_task_impl_t *this) {
     this->scm->free(this->scm);
     this->runnerid->free(this->runnerid);
     this->action->free(this->action);
     // don't free this->run since it belongs to this->action
     free(this);
}

static yacad_task_t impl_fn = {
     .get_id = (yacad_task_get_id_fn)get_id,
     .set_id = (yacad_task_set_id_fn)set_id,
     .get_timestamp = (yacad_task_get_timestamp_fn)get_timestamp,
     .get_status = (yacad_task_get_status_fn)get_status,
     .set_status = (yacad_task_set_status_fn)set_status,
     .get_runnerid = (yacad_task_get_runnerid_fn)get_runnerid,
     .get_scm = (yacad_task_get_scm_fn)get_scm,
     .get_run = (yacad_task_get_run_fn)get_run,
     .same_as = (yacad_task_same_as_fn)same_as,
     .free = (yacad_task_free_fn)free_,
};

yacad_task_t *yacad_task_unserialize(logger_t log, unsigned long id, time_t timestamp, yacad_task_status_t status, char *serial_desc, yacad_runnerid_t *runnerid, int actionindex) {
     yacad_task_impl_t *result = NULL;
     json_input_stream_t *in = new_json_input_stream_from_string(serial_desc, stdlib_memory);
     json_value_t *desc = json_parse(in, NULL, stdlib_memory);
     in->free(in);

     if (desc != NULL) {
          result = malloc(sizeof(yacad_task_impl_t));
          result->fn = impl_fn;
          result->log = log;
          result->id = id;
          result->timestamp = timestamp;
          result->status = status;
          result->desc = desc;
          result->runnerid = runnerid;
          result->actionindex = actionindex;
     }
     return I(result);
}

yacad_task_t *yacad_task_new(logger_t log, json_value_t *action, cad_hash_t *env, const char *root_path) {
     yacad_task_impl_t *result = malloc(sizeof(yacad_task_impl_t));
     yacad_json_template_t *template;
     yacad_json_finder_t *finder = yacad_json_finder_new(log, json_type_object, "%s");

     result->fn = impl_fn;
     result->log = log;
     time(&(result->timestamp));
     result->id = 0;
     result->status = task_new;

     template = yacad_json_template_new(log, env);
     result->action = template->resolve(template, action);
     I(template)->free(I(template));

     finder->visit(finder, result->action, "scm");
     result->scm = yacad_scm_new(log, finder->get_value(finder), root_path);

     finder->visit(finder, result->action, "run");
     result->run = finder->get_object(finder);

     finder->visit(finder, result->action, "runner");
     result->runnerid = yacad_runnerid_new(log, finder->get_value(finder));

     I(finder)->free(I(finder));

     result->runnerid = runnerid;
     return I(result);
}
