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

typedef struct yacad_task_impl_s {
     yacad_task_t fn;
     int id;
     char *project_name;
     char *runner_name;
     char *action;
} yacad_task_impl_t;

static int get_id(yacad_task_impl_t *this) {
     return this->id;
}

static const char *get_project_name(yacad_task_impl_t *this) {
     return this->project_name;
}

static const char *get_runner_name(yacad_task_impl_t *this) {
     return this->runner_name;
}

static const char *get_action(yacad_task_impl_t *this) {
     return this->action;
}

static const char *serialize(yacad_task_impl_t *this) {
     static char result[4096];
     snprintf(result, 4096, "{'id':%d,'project':'%s','runner':'%s','action':'%s'}",
              this->id, this->project_name, this->runner_name,this->action);
     return result;
}

static void free_(yacad_task_impl_t *this) {
     free(this->project_name);
     free(this->runner_name);
     free(this->action);
     free(this);
}

static yacad_task_t impl_fn = {
     .get_id = (yacad_task_get_id_fn)get_id,
     .get_project_name = (yacad_task_get_project_name_fn)get_project_name,
     .get_runner_name = (yacad_task_get_runner_name_fn)get_runner_name,
     .get_action = (yacad_task_get_action_fn)get_action,
     .serialize = (yacad_task_serialize_fn)serialize,
     .free = (yacad_task_free_fn)free_,
};

yacad_task_t *yacad_task_unserialize(const char *serial) {
     // TODO
     return NULL;
}

yacad_task_t *yacad_task_new(int id, const char *project_name, const char *runner_name, const char *action) {
     yacad_task_impl_t*result = malloc(sizeof(yacad_task_impl_t));
     result->fn = impl_fn;
     result->id = id;
     result->project_name = strdup(project_name);
     result->runner_name = strdup(runner_name);
     result->action = strdup(action);
     return I(result);
}
