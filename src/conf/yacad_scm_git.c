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
#include <git2.h>

#include "yacad_scm_git.h"

#define REMOTE_NAME "yacad_upstream"

typedef struct yacad_scm_git_s {
     yacad_scm_t fn;
     yacad_conf_t *conf;
     git_repository *repo;
     git_remote *remote;
     char *root_path; // -> data
     char *upstream_url; // -> data + root_path
     char data[0];
} yacad_scm_git_t;

static bool_t __gitcheck(yacad_conf_t *conf, int giterr, const char *gitaction, unsigned int line) {
     int result = true;
     const git_error *e;
     char *paren;
     int len;
     if (giterr < 0) {
          conf->log(debug, "Error line %u: %s\n", line, gitaction);
          paren = strchrnul(gitaction, '(');
          len = paren - gitaction;
          e = giterr_last();
          if (e == NULL) {
               conf->log(warn, "%.*s: error %d\n", len, gitaction, giterr);
          } else {
               conf->log(warn, "%.*s: error %d/%d: %s\n", len, gitaction, giterr, e->klass, e->message);
          }
          result = false;
     }
     return result;
}

#define gitcheck(conf, gitaction) __gitcheck(conf, (gitaction), #gitaction, __LINE__)

static bool_t check(yacad_scm_git_t *this) {
     /*
      * git remote update
      * git status -uno
      */
     bool_t result = false;
     if (gitcheck(this->conf, git_remote_fetch(this->remote, NULL, NULL))) {
          // TODO
     }
     return result;
}

static void free_(yacad_scm_git_t *this) {
     git_repository_free(this->repo);
     free(this);
}

static yacad_scm_t git_fn = {
     (yacad_scm_check_fn) check,
     (yacad_scm_free_fn) free_,
};

yacad_scm_t *yacad_scm_git_new(yacad_conf_t *conf, const char *root_path, const char *upstream_url) {
     yacad_scm_git_t *result = malloc(sizeof(yacad_scm_git_t) + strlen(root_path) + strlen(upstream_url) + 2);

     result->fn = git_fn;
     result->conf = conf;

     result->repo = NULL;
     result->remote = NULL;
     if (gitcheck(conf, git_repository_open(&(result->repo), root_path))) {
          if (!gitcheck(conf, git_remote_load(&(result->remote), result->repo, REMOTE_NAME))) {
               if (!gitcheck(conf, git_remote_create(&(result->remote), result->repo, REMOTE_NAME, upstream_url))) {
                    git_repository_free(result->repo);
                    free(result);
                    return NULL;
               }
          }
     } else {
          if (mkpath(root_path, 0700)) {
               conf->log(warn, "Could not create directory: %s\n", root_path);
               free(result);
               return NULL;
          }
          if (!gitcheck(conf, git_repository_init(&(result->repo), root_path, false))) {
               free(result);
               return NULL;
          }
          if (!gitcheck(conf, git_remote_create(&(result->remote), result->repo, REMOTE_NAME, upstream_url))) {
               git_repository_free(result->repo);
               free(result);
               return NULL;
          }
     }

     result->root_path = result->data;
     result->upstream_url = result->data + strlen(root_path) + 1;
     strcpy(result->root_path, root_path);
     strcpy(result->upstream_url, upstream_url);
     return I(result);
}
