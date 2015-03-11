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
     int fetch_percent;
     int index_percent;
     char *root_path; // -> data
     char *upstream_url; // -> data + root_path
     char data[0];
} yacad_scm_git_t;

#if LIBGIT2_SOVERSION == 21
static int yacad_init_counter = 0;
#endif

static void yacad_git_init(void) {
#if LIBGIT2_SOVERSION > 21
     git_libgit2_init();
#elif LIBGIT2_SOVERSION > 20
     if (yacad_init_counter++ == 0) {
          git_threads_init();
     }
#endif
}

static void yacad_git_shutdown(void) {
#if LIBGIT2_SOVERSION > 21
     git_libgit2_shutdown();
#elif LIBGIT2_SOVERSION > 20
     if (--yacad_init_counter == 0) {
          git_threads_shutdown();
     }
#endif
}

static bool_t __gitcheck(yacad_conf_t *conf, int giterr, level_t level, const char *gitaction, unsigned int line) {
     int result = true;
     const git_error *e;
     char *paren;
     int len;
     if (giterr != 0) {
          conf->log(debug, "Error line %u: %s\n", line, gitaction);
          paren = strchrnul(gitaction, '(');
          len = paren - gitaction;
          e = giterr_last();
          if (e == NULL) {
               conf->log(level, "%.*s: error %d\n", len, gitaction, giterr);
          } else {
               conf->log(level, "%.*s: error %d/%d: %s\n", len, gitaction, giterr, e->klass, e->message);
          }
          result = false;
     }
     return result;
}

#define gitcheck(conf, gitaction, level) __gitcheck(conf, (gitaction), (level), #gitaction, __LINE__)

static bool_t check(yacad_scm_git_t *this) {
     /*
      * git remote update
      * git status -uno
      */
     bool_t result = false;
     this->fetch_percent = this->index_percent = -1;
     this->conf->log(info, "Updating remote: %s\n", this->upstream_url);
     if (gitcheck(this->conf, git_remote_fetch(this->remote, NULL, NULL), warn)) {
          // TODO
     }
     return result;
}

static void free_(yacad_scm_git_t *this) {
     git_remote_free(this->remote);
     git_repository_free(this->repo);
     free(this);
     yacad_git_shutdown();
}

static int yacad_git_sideband_progress(const char *str, int len, yacad_scm_git_t *this) {
     this->conf->log(debug, "%.*s\n", len, str);
     return 0;
}

static int yacad_git_transfer_progress(const git_transfer_progress *stats, yacad_scm_git_t *this) {
    int fetch_percent = (100 * stats->received_objects) / stats->total_objects;
    int index_percent = (100 * stats->indexed_objects) / stats->total_objects;
    int kbytes = stats->received_bytes / 1024;

    if (fetch_percent != this->fetch_percent || index_percent != this->index_percent) {
         this->conf->log(debug, "Transfer: network %3d%% (%4d Kib, %5d/%5d)  /  indexing %3d%% (%5d/%5d)\n",
                         fetch_percent, kbytes, stats->received_objects, stats->total_objects,
                         index_percent, stats->indexed_objects, stats->total_objects);
         this->fetch_percent = fetch_percent;
         this->index_percent = index_percent;
    }
    return 0;
}

static int yacad_git_credentials(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, yacad_scm_git_t *this) {
     // TODO one of git_cred_ssh_key_from_agent, git_cred_ssh_key_new, git_cred_userpass_plaintext_new, or git_cred_default_new
     this->conf->log(debug, "calling default cred\n");
     return git_cred_default_new(out) || git_cred_ssh_key_from_agent(out, username_from_url);
}

static yacad_scm_t git_fn = {
     (yacad_scm_check_fn) check,
     (yacad_scm_free_fn) free_,
};

yacad_scm_t *yacad_scm_git_new(yacad_conf_t *conf, const char *root_path, json_object_t *desc) {
     size_t sz;
     yacad_scm_git_t *result = NULL;
     git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
     char *upstream_url = NULL;
     size_t szurl = 0, szpath = strlen(root_path) + 1;
     json_string_t *jurl;
     yacad_conf_visitor_t *u = conf->visitor(conf, json_type_string, "upstream_url");

     u->visit(u, (json_value_t *)desc);
     jurl = u->get_string(u);
     if (jurl == NULL) {
          conf->log(warn, "No upstream url\n");
          goto error;
     }
     szurl = jurl->utf8(jurl, "", 0) + 1;
     upstream_url = malloc(szurl);
     jurl->utf8(jurl, upstream_url, szurl);

     sz = sizeof(yacad_scm_git_t) + szpath + szurl;
     result = malloc(sz);

     result->fn = git_fn;
     result->conf = conf;
     result->repo = NULL;
     result->remote = NULL;

     yacad_git_init();

     if (gitcheck(conf, git_repository_open(&(result->repo), root_path), debug)) {
          if (gitcheck(conf, git_remote_load(&(result->remote), result->repo, REMOTE_NAME), debug)) {
               if (!gitcheck(conf, git_remote_delete(result->remote), warn)) {
                    goto error;
               }
               git_remote_free(result->remote);
          }
     } else {
          if (mkpath(root_path, 0700)) {
               conf->log(warn, "Could not create directory: %s\n", root_path);
               goto error;
          }
          if (!gitcheck(conf, git_repository_init(&(result->repo), root_path, false), warn)) {
               goto error;
          }
     }
     if (!gitcheck(conf, git_remote_create(&(result->remote), result->repo, REMOTE_NAME, upstream_url), warn)) {
          goto error;
     }

     callbacks.sideband_progress = (git_transport_message_cb)yacad_git_sideband_progress;
     callbacks.transfer_progress = (git_transfer_progress_cb)yacad_git_transfer_progress;
     callbacks.credentials  = (git_cred_acquire_cb)yacad_git_credentials;
     callbacks.payload = result;
     if (!gitcheck(conf, git_remote_set_callbacks(result->remote, &callbacks), warn)) {
          goto error;
     }

     result->fetch_percent = result->index_percent = -1;
     result->root_path = result->data;
     result->upstream_url = result->data + szpath;
     strncpy(result->root_path, root_path, szpath);
     strncpy(result->upstream_url, upstream_url, szurl);

ret:
     free(upstream_url);
     I(u)->free(I(u));
     return I(result);

error:
     if (result) {
          git_remote_free(result->remote);
          git_repository_free(result->repo);
          free(result);
          result = NULL;
     }
     goto ret;
}
