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

#include "yacad_project.h"

typedef struct yacad_project_impl_s {
     yacad_project_t fn;
     logger_t log;
     yacad_cron_t *cron;
     json_array_t *tasks;
     yacad_scm_t *scm;
     char *name;
     char *root_path;
     char _[0];
} yacad_project_impl_t;

static const char *get_name(yacad_project_impl_t *this) {
     return this->name;
}

static struct timeval next_check(yacad_project_impl_t *this) {
     struct timeval result = this->cron->next(this->cron);
     char tmbuf[20];
     this->log(debug, "Project \"%s\" next check time: %s", this->name, datetime(result.tv_sec, tmbuf));
     return result;
}

static void env_cleaner(cad_hash_t *env, int index, const char *key, char *value, yacad_project_impl_t *this) {
     free(value);
}

static yacad_task_t *check(yacad_project_impl_t *this) {
     yacad_task_t *result = NULL;
     cad_hash_t *env = cad_new_hash(stdlib_memory, cad_hash_strings);
     if (this->scm->check(this->scm)) {
          this->scm->fill_env(this->scm, env);
          result = yacad_task_new(this->log, this->tasks->get(this->tasks, 0), env, this->name, 0);
     }
     env->clean(env, (cad_hash_iterator_fn)env_cleaner, this);
     env->free(env);
     return result;
}

static yacad_task_t *next_task(yacad_project_impl_t *this, yacad_task_t *previous) {
     yacad_task_t *result = NULL;
     int taskindex = previous->get_taskindex(previous) + 1;
     cad_hash_t *env = previous->get_env(previous);
     if (taskindex < this->tasks->count(this->tasks)) {
          result = yacad_task_new(this->log, this->tasks->get(this->tasks, taskindex), env, this->name, taskindex);
     }
     return result;
}

static void free_(yacad_project_impl_t *this) {
     this->cron->free(this->cron);
     free(this);
}

static yacad_project_t impl_fn = {
     .get_name = (yacad_project_get_name_fn) get_name,
     .next_check = (yacad_project_next_check_fn) next_check,
     .check = (yacad_project_check_fn) check,
     .next_task = (yacad_project_next_task_fn)next_task,
     .free = (yacad_project_free_fn) free_,
};

yacad_project_t *yacad_project_new(logger_t log, const char *name, const char *root_path, yacad_cron_t *cron, yacad_scm_t *scm, json_array_t *tasks) {
     size_t szname = strlen(name) + 1;
     size_t szroot_path = strlen(root_path) + 1;
     yacad_project_impl_t *result = malloc(sizeof(yacad_project_impl_t) + szname + szroot_path);
     result->fn = impl_fn;
     result->log = log;
     result->name = result->_;
     strcpy(result->name, name);
     result->root_path = result->_ + szname;
     strcpy(result->root_path, root_path);
     result->cron = cron;
     result->tasks = tasks;
     result->scm = scm;
     return I(result);
}
