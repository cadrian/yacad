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

#include "yacad_scm_git.h"

typedef struct yacad_scm_git_s {
     yacad_scm_t fn;
     char *root_path; // -> data
     char *upstream_url; // -> data + root_path
     char data[0];
} yacad_scm_git_t;

static bool_t check(yacad_scm_git_t *this) {
     return false;
}

static void free_(yacad_scm_git_t *this) {
     free(this);
}

static yacad_scm_t git_fn = {
     (yacad_scm_check_fn) check,
     (yacad_scm_free_fn) free_,
};

yacad_scm_t *yacad_scm_git_new(yacad_conf_t *conf, const char *root_path, const char *upstream_url) {
     yacad_scm_git_t *result = malloc(sizeof(yacad_scm_git_t) + strlen(root_path) + strlen(upstream_url) + 2);
     result->fn = git_fn;
     result->root_path = result->data;
     result->upstream_url = result->data + strlen(root_path) + 1;
     strcpy(result->root_path, root_path);
     strcpy(result->upstream_url, upstream_url);
     return I(result);
}
