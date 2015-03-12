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

#include "yacad_tasklist.h"

typedef struct yacad_tasklist_impl_s {
     yacad_tasklist_t fn;
     logger_t log;
     cad_array_t *tasklist;
} yacad_tasklist_impl_t;

static void add(yacad_tasklist_impl_t *this, yacad_task_t *task) {
     int i, n;
     bool_t found = false;
     yacad_task_t *other;
     n = this->tasklist->count(this->tasklist);
     for (i = 0; !found && i < n; i++) {
          other = *(yacad_task_t**)this->tasklist->get(this->tasklist, i);
          found = task->same_as(task, other);
     }
     if (found) {
          this->log(debug, "Task not added: %s\n", task->serialize(task));
          task->free(task);
     } else {
          this->tasklist->insert(this->tasklist, n, &task);
          this->log(info, "Added task: %s\n", task->serialize(task));
          // TODO if task.id == 0??
          // TODO save to database
     }
}

static yacad_task_t *get(yacad_tasklist_impl_t *this, yacad_runner_t *runner) {
     yacad_task_t *result = NULL;
     int index = 0; // TODO find the best matching index
     result = *(yacad_task_t **)this->tasklist->get(this->tasklist, index);
     this->tasklist->del(this->tasklist, index);
     // TODO save to database // NO!! do that only when the task is successfully performed (we need an extra method here)
     return result;
}

static void free_(yacad_tasklist_impl_t *this) {
     int i, n = this->tasklist->count(this->tasklist);
     yacad_task_t *task;
     for (i = 0; i < n; i++) {
          task = *(yacad_task_t **)this->tasklist->get(this->tasklist, i);
          task->free(task);
     }
     this->tasklist->free(this->tasklist);
     free(this);
}

static yacad_tasklist_t impl_fn = {
     .add = (yacad_tasklist_add_fn)add,
     .get = (yacad_tasklist_get_fn)get,
     .free = (yacad_tasklist_free_fn)free_,
};

yacad_tasklist_t *yacad_tasklist_new(logger_t log) {
     yacad_tasklist_impl_t *result = malloc(sizeof(yacad_tasklist_impl_t));
     result->fn = impl_fn;
     result->log = log;
     result->tasklist = cad_new_array(stdlib_memory, sizeof(yacad_task_t*));
     // TODO read from database
     return I(result);
}
