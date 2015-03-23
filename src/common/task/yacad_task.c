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
     .same_as = (yacad_task_same_as_fn)same_as,
     .free = (yacad_task_free_fn)free_,
};

yacad_task_t *yacad_task_restore(logger_t log, unsigned long id, time_t timestamp, yacad_task_status_t status, json_value_t *run, json_value_t *source, yacad_runnerid_t *runnerid, const char *project_name, int taskindex) {
     yacad_task_impl_t *result = malloc(sizeof(yacad_task_impl_t) + strlen(project_name) + 1);

     result->fn = impl_fn;
     result->log = log;
     result->id = id;
     result->timestamp = timestamp;
     result->status = status;
     result->task = NULL;
     result->run = run;
     result->source = source;
     result->runnerid = runnerid;
     strcpy(result->project_name, project_name);
     result->taskindex = taskindex;

     return I(result);
}

static void fill_env(cad_hash_t *env, int index, const char *key, const char *value, yacad_task_impl_t *this) {
     this->env->set(this->env, key, strdup(value));
}

yacad_task_t *yacad_task_new(logger_t log, json_value_t *task, cad_hash_t *env, const char *project_name, int taskindex) {
     yacad_task_impl_t *result = malloc(sizeof(yacad_task_impl_t) + strlen(project_name) + 1);
     yacad_json_template_t *template;
     yacad_json_finder_t *finder = yacad_json_finder_new(log, json_type_object, "%s");

     result->fn = impl_fn;
     result->log = log;
     time(&(result->timestamp));
     result->id = 0;
     result->status = task_new;

     result->env = cad_new_hash(stdlib_memory, cad_hash_strings);
     if (env != NULL) {
          env->iterate(env, (cad_hash_iterator_fn)fill_env, result);
     }

     template = yacad_json_template_new(log, result->env);
     result->task = template->resolve(template, task);
     I(template)->free(I(template));

     finder->visit(finder, result->task, "source");
     result->source = finder->get_value(finder);

     finder->visit(finder, result->task, "run");
     result->run = finder->get_value(finder);

     finder->visit(finder, result->task, "runner");
     result->runnerid = yacad_runnerid_new(log, finder->get_value(finder));

     strcpy(result->project_name, project_name);

     result->taskindex = taskindex;

     I(finder)->free(I(finder));

     return I(result);
}
