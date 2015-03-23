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
#include "common/json/yacad_json_compare.h"
#include "common/json/yacad_json_finder.h"
#include "common/json/yacad_json_template.h"

typedef struct yacad_task_impl_s {
     yacad_task_t fn;
     logger_t log;
     unsigned long id;
     time_t timestamp;
     yacad_task_status_t status;
     int taskindex;
     json_value_t *task;
     json_value_t *source;
     json_value_t *run;
     yacad_runnerid_t *runnerid;
     cad_hash_t *env;
     char project_name[0];
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

static json_value_t *get_source(yacad_task_impl_t *this) {
     return this->source;
}

static json_value_t *get_run(yacad_task_impl_t *this) {
     return this->run;
}

static const char *get_project_name(yacad_task_impl_t *this) {
     return this->project_name;
}

static int get_taskindex(yacad_task_impl_t *this) {
     return this->taskindex;
}

static void set_taskindex(yacad_task_impl_t *this, int index) {
     this->taskindex = index;
}

static void fill_jenv(cad_hash_t *env, int index, const void *key, const char *value, json_object_t *jenv) {
     json_string_t *data = json_new_string(stdlib_memory);
     data->add_string(data, "%s", value);
     jenv->set(jenv, key, (json_value_t*)data);
}

static char *serialize(yacad_task_impl_t *this) {
     char *result = NULL;
     int n;
     json_object_t *jenv = json_new_object(stdlib_memory);
     char *stask, *senv;
     json_output_stream_t *out_task = new_json_output_stream_from_string(&stask, stdlib_memory),
          *out_env = new_json_output_stream_from_string(&senv, stdlib_memory);
     json_visitor_t *wtask = json_write_to(out_task, stdlib_memory, 0),
          *wenv = json_write_to(out_env, stdlib_memory, 0);

     this->task->accept(this->task, wtask);
     this->env->iterate(this->env, (cad_hash_iterator_fn)fill_jenv, jenv);
     jenv->accept(jenv, wenv);

     n = 1 + snprintf(
          "", 0,
          "{\"id\":%lu,\"timestamp\":%lu,\"status\":%d,\"taskindex\":%d,\"task\":%s\"project_name\":\"%s\",\"env\":%s}",
          this->id, (unsigned long)this->timestamp, (int)this->status, this->taskindex, stask, this->project_name, senv);
     result = malloc(n);
     snprintf(
          result, n,
          "{\"id\":%lu,\"timestamp\":%lu,\"status\":%d,\"taskindex\":%d,\"task\":%s\"project_name\":\"%s\",\"env\":%s}",
          this->id, (unsigned long)this->timestamp, (int)this->status, this->taskindex, stask, this->project_name, senv);

     jenv->accept(jenv, json_kill());

     free(stask);
     free(senv);
     out_task->free(out_task);
     out_env->free(out_env);
     wtask->free(wtask);
     wenv->free(wenv);

     return result;
}

static cad_hash_t *get_env(yacad_task_impl_t *this) {
     return this->env;
}

static bool_t same_as(yacad_task_impl_t *this, yacad_task_impl_t *other) {
     bool_t result = false;
     static yacad_json_compare_t *cmp = NULL;
     if (cmp == NULL) {
          cmp = yacad_json_compare_new(this->log);
     }

     if (this->runnerid->same_as(this->runnerid, other->runnerid)) {
          if (this->task != NULL) {
               if (other->task != NULL) {
                    result = cmp->equal(cmp, this->task, other->task);
               }
          } else {
               result = cmp->equal(cmp, this->source, other->source)
                    && cmp->equal(cmp, this->run, other->run);
          }
     }
     return result;
}

static void env_cleaner(cad_hash_t *env, int index, const char *key, char *value, yacad_task_impl_t *this) {
     free(value);
}

static void free_(yacad_task_impl_t *this) {
     this->env->clean(this->env, (cad_hash_iterator_fn)env_cleaner, this);
     this->env->free(this->env);
     free(this);
}

static yacad_task_t impl_fn = {
     .get_id = (yacad_task_get_id_fn)get_id,
     .set_id = (yacad_task_set_id_fn)set_id,
     .get_timestamp = (yacad_task_get_timestamp_fn)get_timestamp,
     .get_status = (yacad_task_get_status_fn)get_status,
     .set_status = (yacad_task_set_status_fn)set_status,
     .get_runnerid = (yacad_task_get_runnerid_fn)get_runnerid,
     .get_source = (yacad_task_get_source_fn)get_source,
     .get_run = (yacad_task_get_run_fn)get_run,
     .get_project_name = (yacad_task_get_project_name_fn)get_project_name,
     .get_taskindex = (yacad_task_get_taskindex_fn)get_taskindex,
     .set_taskindex = (yacad_task_set_taskindex_fn)set_taskindex,
     .get_env = (yacad_task_get_env_fn)get_env,
     .serialize = (yacad_task_serialize_fn)serialize,
     .same_as = (yacad_task_same_as_fn)same_as,
     .free = (yacad_task_free_fn)free_,
};

static yacad_task_impl_t *_task_new(logger_t log, json_value_t *task, const char *project_name, int taskindex) {
     yacad_task_impl_t *result = malloc(sizeof(yacad_task_impl_t) + strlen(project_name) + 1);
     yacad_json_finder_t *finder = yacad_json_finder_new(log, json_type_object, "%s");

     result->fn = impl_fn;
     result->log = log;
     time(&(result->timestamp));

     result->task = task;
     finder->visit(finder, task, "source");
     result->source = finder->get_value(finder);
     finder->visit(finder, task, "run");
     result->run = finder->get_value(finder);
     finder->visit(finder, task, "runner");
     result->runnerid = yacad_runnerid_new(log, finder->get_value(finder));

     strcpy(result->project_name, project_name);

     result->taskindex = taskindex;

     I(finder)->free(I(finder));

     return result;
}

static void fill_env(cad_hash_t *env, int index, const char *key, const char *value, cad_hash_t *resolved_env) {
     resolved_env->set(resolved_env, key, strdup(value));
}

yacad_task_t *yacad_task_new(logger_t log, json_value_t *task, cad_hash_t *env, const char *project_name, int taskindex) {
     yacad_task_impl_t *result;
     yacad_json_template_t *template;
     cad_hash_t *resolved_env;
     json_value_t *resolved_task;

     resolved_env = cad_new_hash(stdlib_memory, cad_hash_strings);
     if (env != NULL) {
          env->iterate(env, (cad_hash_iterator_fn)fill_env, resolved_env);
     }

     template = yacad_json_template_new(log, resolved_env);
     resolved_task = template->resolve(template, task);
     I(template)->free(I(template));

     result = _task_new(log, resolved_task, project_name, taskindex);
     result->id = 0;
     time(&(result->timestamp));
     result->status = task_new;
     result->env = resolved_env;

     return I(result);
}

yacad_task_t *yacad_task_unserialize(logger_t log, char *serial) {
     yacad_task_impl_t *result;
     json_input_stream_t *ser = new_json_input_stream_from_string(serial, stdlib_memory);
     json_object_t *jserial = (json_object_t *)json_parse(ser, NULL, stdlib_memory);
     json_number_t *jid = (json_number_t *)jserial->get(jserial, "id");
     json_number_t *jtimestamp = (json_number_t *)jserial->get(jserial, "timestamp");
     json_number_t *jstatus = (json_number_t *)jserial->get(jserial, "status");
     json_number_t *jtaskindex = (json_number_t *)jserial->get(jserial, "taskindex");
     json_object_t *jtask = (json_object_t *)jserial->get(jserial, "task");
     json_string_t *jproject_name = (json_string_t *)jserial->get(jserial, "project_name");
     json_object_t *jenv = (json_object_t *)jserial->get(jserial, "env");
     json_string_t *jstring;
     char *project_name;
     int i, n, c;
     int taskindex;
     const char **keys;
     char *key;

     c = jproject_name->utf8(jproject_name, "", 0) + 1;
     project_name = malloc(c);
     jproject_name->utf8(jproject_name, project_name, c);

     taskindex = (int)jtaskindex->to_int(jtaskindex);

     result = (yacad_task_impl_t *)yacad_task_new(log, (json_value_t *)jtask, NULL, project_name, taskindex);
     result->id = (unsigned long)jid->to_int(jid);
     result->timestamp = (time_t)jtimestamp->to_int(jtimestamp);
     result->status = (yacad_task_status_t)jstatus->to_int(jstatus);
     result->env = cad_new_hash(stdlib_memory, cad_hash_strings);
     n = jenv->count(jenv);
     if (n > 0) {
          keys = malloc(n * sizeof(char*));
          jenv->keys(jenv, keys);
          for (i = 0; i < n; i++) {
               jstring = (json_string_t *)jenv->get(jenv, keys[i]);
               c = jstring->utf8(jstring, "", 0) + 1;
               key = malloc(c);
               jstring->utf8(jstring, key, c);
               result->env->set(result->env, keys[i], key);
          }
     }

     jserial->accept(jserial, json_kill());

     return I(result);
}
