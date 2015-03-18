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
     json_array_t *actions;
     char name[0];
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

static yacad_task_t *check(yacad_project_impl_t *this) {
     return this->scm->check(this->scm);
}

static json_array_t *get_actions(yacad_project_impl_t *this) {
     return this->actions;
}

static void free_(yacad_project_impl_t *this) {
     this->cron->free(this->cron);
     free(this);
}

static yacad_project_t impl_fn = {
     .get_name = (yacad_project_get_name_fn) get_name,
     .next_check = (yacad_project_next_check_fn) next_check,
     .check = (yacad_project_check_fn) check,
     .get_actions = (yacad_get_actions_fn)get_actions,
     .free = (yacad_project_free_fn) free_,
};

yacad_project_t *yacad_project_new(logger_t log, const char *name, yacad_cron_t *cron, json_array_t *actions) {
     yacad_project_impl_t *result = malloc(sizeof(yacad_project_impl_t) + strlen(name) + 1);
     result->fn = impl_fn;
     result->log = log;
     strcpy(result->name, name);
     result->cron = cron;
     result->actions = actions;
     return I(result);
}
