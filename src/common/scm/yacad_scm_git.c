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

#include <git2.h>

#include "yacad_scm_git.h"
#include "common/json/yacad_json_finder.h"

#define REMOTE_NAME "yacad_upstream"

typedef struct yacad_scm_git_s {
     yacad_scm_t fn;
     logger_t log;
     git_repository *repo;
     git_remote *remote;
     int fetch_percent;
     int index_percent;
     json_value_t *desc;
     char *root_path; // -> data
     char *upstream_url; // -> data + root_path
     char data[0];
} yacad_scm_git_t;

#if LIBGIT2_SOVERSION == 21
static int yacad_init_counter = 0;
#endif

static void yacad_git_init(void) {
#if LIBGIT2_SOVERSION >= 22
     git_libgit2_init();
#elif LIBGIT2_SOVERSION == 21
     if (yacad_init_counter++ == 0) {
          git_threads_init();
     }
#endif
}

static void yacad_git_shutdown(void) {
#if LIBGIT2_SOVERSION >= 22
     git_libgit2_shutdown();
#elif LIBGIT2_SOVERSION == 21
     if (--yacad_init_counter == 0) {
          git_threads_shutdown();
     }
#endif
}

static bool_t __gitcheck(logger_t log, int giterr, level_t level, const char *gitaction, unsigned int line) {
     int result = true;
     const git_error *e;
     char *paren;
     int len;
     if (giterr != 0) {
          log(debug, "Error line %u: %s", line, gitaction);
          paren = strchrnul(gitaction, '(');
          len = paren - gitaction;
          e = giterr_last();
          if (e == NULL) {
               log(level, "%.*s: error %d", len, gitaction, giterr);
          } else {
               log(level, "%.*s: error %d/%d: %s", len, gitaction, giterr, e->klass, e->message);
          }
          result = false;
     }
     return result;
}

#define gitcheck(log, gitaction, level) __gitcheck(log, (gitaction), (level), #gitaction, __LINE__)

static yacad_task_t *build_task(yacad_scm_git_t *this){
     yacad_task_t *result = NULL;
     cad_hash_t *env = cad_new_hash(stdlib_memory, cad_hash_strings);
     const git_remote_head **heads;
     git_buf branch = {NULL,0,0};
     size_t szheads = 0;
     char ref[GIT_OID_HEXSZ + 1];

     if (gitcheck(this->log, git_remote_ls(&heads, &szheads, this->remote), warn) && szheads > 0) {
          // The first one is HEAD
          git_oid_tostr(ref, GIT_OID_HEXSZ + 1, &heads[0]->oid);
     }
     gitcheck(this->log, git_remote_default_branch(&branch, this->remote), warn);

     env->set(env, "ref", ref);
     env->set(env, "branch", branch.ptr);

     result = yacad_task_new(this->log, this->desc, env);

     env->free(env);
     free(branch.ptr);
     return result;
}

static yacad_task_t *check(yacad_scm_git_t *this) {
     yacad_task_t *result = NULL;
     bool_t downloaded;

     if (!gitcheck(this->log, git_remote_connect(this->remote, GIT_DIRECTION_FETCH), warn)) {
          // Could not connect to remote
     } else {
          this->fetch_percent = this->index_percent = -1;
          downloaded = gitcheck(this->log, git_remote_download(this->remote), warn);
          git_remote_disconnect(this->remote);
          if (downloaded) {
               if (!gitcheck(this->log, git_remote_update_tips(this->remote, NULL, NULL), warn)) {
                    // Could not update tips
               } else if (this->fetch_percent == -1 && this->index_percent == -1) {
                    this->log(debug, "Remote is up-to-date: %s", this->upstream_url);
               } else if (this->fetch_percent != 100 || this->index_percent != 100) {
                    this->log(warn, "Download incomplete: network %3d%%  /  indexing %3d%%", this->fetch_percent, this->index_percent);
               } else {
                    this->log(info, "Remote needs building: %s", this->upstream_url);
                    result = build_task(this);
               }
          }
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
     this->log(debug, "%.*s", len, str);
     return 0;
}

static int yacad_git_transfer_progress(const git_transfer_progress *stats, yacad_scm_git_t *this) {
    int fetch_percent = (100 * stats->received_objects) / stats->total_objects;
    int index_percent = (100 * stats->indexed_objects) / stats->total_objects;
    int kbytes = stats->received_bytes / 1024;

    if (fetch_percent != this->fetch_percent || index_percent != this->index_percent) {
         this->log(debug, "Transfer: network %3d%% (%4d Kib, %5d/%5d)  /  indexing %3d%% (%5d/%5d)",
                   fetch_percent, kbytes, stats->received_objects, stats->total_objects,
                   index_percent, stats->indexed_objects, stats->total_objects);
         this->fetch_percent = fetch_percent;
         this->index_percent = index_percent;
    }
    return 0;
}

static int yacad_git_credentials(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, yacad_scm_git_t *this) {
     // TODO one of git_cred_ssh_key_from_agent, git_cred_ssh_key_new, git_cred_userpass_plaintext_new, or git_cred_default_new
     this->log(debug, "calling default cred");
     if (git_cred_default_new(out) == 0) {
          return 0;
     }
     return git_cred_ssh_key_from_agent(out, username_from_url);
}

static yacad_scm_t git_fn = {
     (yacad_scm_check_fn) check,
     (yacad_scm_free_fn) free_,
};

yacad_scm_t *yacad_scm_git_new(logger_t log, const char *root_path, json_value_t *desc) {
     size_t sz;
     yacad_scm_git_t *result = NULL;
     git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
     char *upstream_url = NULL;
     size_t szurl = 0, szpath = strlen(root_path) + 5;
     json_string_t *jurl;
     yacad_json_finder_t *u = yacad_json_finder_new(log, json_type_string, "upstream_url");

     u->visit(u, (json_value_t *)desc);
     jurl = u->get_string(u);
     if (jurl == NULL) {
          log(warn, "No upstream url");
          goto error;
     }
     szurl = jurl->utf8(jurl, "", 0) + 1;
     upstream_url = malloc(szurl);
     jurl->utf8(jurl, upstream_url, szurl);

     sz = sizeof(yacad_scm_git_t) + szpath + szurl;
     result = malloc(sz);

     result->fn = git_fn;
     result->log = log;
     result->repo = NULL;
     result->remote = NULL;
     result->desc = desc;

     result->fetch_percent = result->index_percent = -1;
     result->root_path = result->data;
     result->upstream_url = result->data + szpath;
     snprintf(result->root_path, szpath, "%s.git", root_path);
     snprintf(result->upstream_url, szurl, "%s", upstream_url);

     yacad_git_init();

     if (gitcheck(log, git_repository_open_bare(&(result->repo), result->root_path), debug)) {
          if (gitcheck(log, git_remote_load(&(result->remote), result->repo, REMOTE_NAME), debug)) {
               if (!gitcheck(log, git_remote_delete(result->remote), warn)) {
                    goto error;
               }
               git_remote_free(result->remote);
          }
     } else {
          log(info, "Initializing repository: %s", result->root_path);
          if (!gitcheck(log, git_repository_init(&(result->repo), result->root_path, true), warn)) {
               goto error;
          }
     }
     if (!gitcheck(log, git_remote_create(&(result->remote), result->repo, REMOTE_NAME, upstream_url), warn)) {
          goto error;
     }

     callbacks.sideband_progress = (git_transport_message_cb)yacad_git_sideband_progress;
     callbacks.transfer_progress = (git_transfer_progress_cb)yacad_git_transfer_progress;
     callbacks.credentials  = (git_cred_acquire_cb)yacad_git_credentials;
     callbacks.payload = result;
     if (!gitcheck(log, git_remote_set_callbacks(result->remote, &callbacks), warn)) {
          goto error;
     }

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
