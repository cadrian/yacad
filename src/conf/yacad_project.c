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

#include <string.h>

#include "yacad_project.h"

typedef struct yacad_project_impl_s {
     yacad_project_t fn;
     yacad_conf_t *conf;
     char *name;
     char *scm;
     char *root_path;
     char *upstream_url;
     char *cron;
} yacad_project_impl_t;

static struct timeval next_check(yacad_project_impl_t *this) {
     struct timeval tm;
     gettimeofday(&tm, NULL);
     tm.tv_usec = 0;
     tm.tv_sec++; // TODO: parse cron
     this->conf->log(debug, "next_check %s\n", this->name);
     return tm;
}

static bool_t *check(yacad_project_impl_t *this) {
     return false;
}

static void free_(yacad_project_impl_t *this) {
     free(this->name);
     free(this->scm);
     free(this->root_path);
     free(this->upstream_url);
     free(this->cron);
     free(this);
}

static yacad_project_t impl_fn = {
     (yacad_project_next_check_fn) next_check,
     (yacad_project_check_fn) check,
     (yacad_project_free_fn) free_,
};

yacad_project_t *yacad_project_new(yacad_conf_t *conf, const char *name, const char *scm, const char *root_path, const char *upstream_url, const char *cron) {
     yacad_project_impl_t *result = malloc(sizeof(yacad_project_impl_t));
     result->fn = impl_fn;
     result->conf = conf;
     result->name = strdup(name);
     result->scm = strdup(scm);
     result->root_path = strdup(root_path);
     result->upstream_url = strdup(upstream_url);
     result->cron = strdup(cron);
     return I(result);
}
