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
     json_value_t *desc;
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
     this->desc->free(this->desc);
     free(this);
}

static yacad_task_t impl_fn = {
     .get_id = (yacad_task_get_id_fn)get_id,
     .set_id = (yacad_task_set_id_fn)set_id,
     .get_timestamp = (yacad_task_get_timestamp_fn)get_timestamp,
     .get_status = (yacad_task_get_status_fn)get_status,
     .set_status = (yacad_task_set_status_fn)set_status,
     .serialize = (yacad_task_serialize_fn)serialize,
     .same_as = (yacad_task_same_as_fn)same_as,
     .free = (yacad_task_free_fn)free_,
};

yacad_task_t *yacad_task_unserialize(logger_t log, unsigned long id, time_t timestamp, yacad_task_status_t status, char *serial) {
     yacad_task_impl_t *result = NULL;
     json_input_stream_t *in = new_json_input_stream_from_string(serial, stdlib_memory);
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
     }
     return I(result);
}

yacad_task_t *yacad_task_new(logger_t log, json_value_t *desc, cad_hash_t *env) {
     yacad_task_impl_t *result = malloc(sizeof(yacad_task_impl_t));
     yacad_json_template_t *template;
     result->fn = impl_fn;
     result->log = log;
     time(&(result->timestamp));
     result->id = 0;
     result->status = task_new;
     if (env == NULL) {
          result->desc = desc;
     } else {
          template = yacad_json_template_new(log, env);
          result->desc = template->resolve(template, desc);
          I(template)->free(I(template));
     }
     return I(result);
}
