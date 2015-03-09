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

#include <stdio.h>
#include <string.h>

#include <cad_array.h>
#include <cad_hash.h>

#include "yacad_tasklist.h"

typedef struct yacad_tasklist_impl_s {
     yacad_tasklist_t fn;
     yacad_conf_t *conf;
     cad_hash_t *tasklist_per_runner;
} yacad_tasklist_impl_t;

static bool_t same_task(yacad_task_t *task1, yacad_task_t *task2) {
     bool_t result = true;
     if (strcmp(task1->get_project_name(task1), task2->get_project_name(task2))) {
          result = false;
     } else if (strcmp(task1->get_runner_name(task1), task2->get_runner_name(task2))) {
          result = false;
     } else if (strcmp(task1->get_action(task1), task2->get_action(task2))) {
          result = false;
     }
     return result;
}

static void add(yacad_tasklist_impl_t *this, yacad_task_t *task) {
     const char *runner_name = task->get_runner_name(task);
     cad_array_t *tasklist = this->tasklist_per_runner->get(this->tasklist_per_runner, runner_name);
     int i, n;
     bool_t found = false;
     if (tasklist == NULL) {
          tasklist = cad_new_array(stdlib_memory, sizeof(yacad_task_t*));
          this->tasklist_per_runner->set(this->tasklist_per_runner, runner_name, tasklist);
     }
     n = tasklist->count(tasklist);
     for (i = 0; !found && i < n; i++) {
          found = same_task(task, *(yacad_task_t**)tasklist->get(tasklist, i));
     }
     if (found) {
          this->conf->log(debug, "task not added: %s\n", task->serialize(task));
          task->free(task);
     } else {
          tasklist->insert(tasklist, n, &task);
          this->conf->log(info, "added task: %s\n", task->serialize(task));
          // TODO if task.id == 0??
          // TODO save to database
     }
}

static yacad_task_t *get(yacad_tasklist_impl_t *this, const char *runner_name) {
     yacad_task_t *result = NULL;
     cad_array_t *tasklist = this->tasklist_per_runner->get(this->tasklist_per_runner, runner_name);
     if (tasklist != NULL) {
          result = *(yacad_task_t **)tasklist->get(tasklist, 0);
          tasklist->del(tasklist, 0);
     }
     // TODO save to database // NO!! do that only when the task is successfully performed (we need an extra method here)
     return result;
}

static void free_tasklist(cad_hash_t *dict, int index, const char *runner_name, cad_array_t *tasklist,
                          yacad_tasklist_impl_t *this) {
     int i, n = tasklist->count(tasklist);
     yacad_task_t *task;
     for (i = 0; i < n; i++) {
          task = *(yacad_task_t **)tasklist->get(tasklist, i);
          task->free(task);
     }
     tasklist->free(tasklist);
}

static void free_(yacad_tasklist_impl_t *this) {
     this->tasklist_per_runner->clean(this->tasklist_per_runner, (cad_hash_iterator_fn)free_tasklist, this);
     this->tasklist_per_runner->free(this->tasklist_per_runner);
     free(this);
}

static yacad_tasklist_t impl_fn = {
     .add = (yacad_tasklist_add_fn)add,
     .get = (yacad_tasklist_get_fn)get,
     .free = (yacad_tasklist_free_fn)free_,
};

yacad_tasklist_t *yacad_tasklist_new(yacad_conf_t *conf) {
     yacad_tasklist_impl_t *result = malloc(sizeof(yacad_tasklist_impl_t));
     result->fn = impl_fn;
     result->conf = conf;
     result->tasklist_per_runner = cad_new_hash(stdlib_memory, cad_hash_strings);
     // TODO read from database
     return &(result->fn);
}
