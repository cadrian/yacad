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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "yacad_event.h"

typedef struct yacad_event_impl_s {
     yacad_event_t fn;
     yacad_event_action action;
     yacad_event_callback callback;
     int timeout;
     void *action_data;
     void *callback_data;
} yacad_event_impl_t;

static void run(yacad_event_impl_t *this) {
     this->action(&(this->fn), this->callback, this->action_data);
}

static void free_(yacad_event_impl_t *this) {
     free(this);
}

static yacad_event_t impl_fn = {
     .run = (yacad_event_run_fn)run,
     .free = (yacad_event_free_fn)free_,
};

yacad_task_t *new_task(char *buffer) {
     char *project_name;
     char *runner_name;
     char *action;
     char *junk;
     if (buffer[0] == '\0') {
          return NULL;
     }
     project_name = buffer;
     runner_name = strchr(project_name, '\n');
     if (runner_name == NULL) {
          runner_name = action = "";
     } else {
          *runner_name = '\0';
          runner_name++;
          action = strchr(runner_name, '\n');
          if (action == NULL) {
               action = "";
          } else {
               *action = '\0';
               action++;
               junk = strchr(action, '\n');
               if (junk != NULL) {
                    *junk = '\0';
               }
          }
     }
     return yacad_task_new(0, project_name, runner_name, action);
}

static char *read_fd(int fd, const char *script) {
     char *buffer;
     int i = 0, n;
     size_t c;
     bool_t done = false;

     buffer = malloc(c = 4096);
     do {
          if (i == c) {
               buffer = realloc(buffer, c *= 2);
          }
          n = read(fd, buffer + i, c - i);
          if (n == 0) {
               done = true;
          } else if (n < 0) {
               fprintf(stderr, "**** %s: error read: %s\n", script, strerror(errno));
               done = true;
          } else {
               i += n;
          }
     } while (!done);
     buffer[i] = '\0';

     return buffer;
}

static void script_action(yacad_event_impl_t *this, yacad_event_callback callback, yacad_conf_t *conf) {
     const char *script = conf->get_action_script(conf);
     int res;
     int pid;
     int io[2];
     char *buffer;
     yacad_task_t *task;
     res = pipe(io);
     if (res < 0) {
          fprintf(stderr, "**** pipe(2): %s: %s\n", script, strerror(errno));
     } else {
          pid = fork();

          if (pid == 0) {
               // in script
               res = close(io[0]);
               if (res < 0) {
                    fprintf(stderr, "**** close(2): %s: %s\n", script, strerror(errno));
                    exit(1);
               }
               res = dup2(io[1], 1);
               if (res < 0) {
                    fprintf(stderr, "**** dup2(2): %s: %s\n", script, strerror(errno));
                    exit(1);
               }
               execlp(script, script, NULL);
               fprintf(stderr, "**** execlp(2): %s: %s\n", script, strerror(errno));
               exit(1);
          }

          // in core
          res = close(io[1]);
          if (res < 0) {
               fprintf(stderr, "**** close(2): %s: %s\n", script, strerror(errno));
               exit(1);
          }
          buffer=read_fd(io[0], script);
          waitpid(pid, &res, 0);
          if (WIFEXITED(res)) {
               task = new_task(buffer);
               if (task != NULL) {
                    callback(&(this->fn), task, this->callback_data);
               }
          } else if (WIFSIGNALED(res)) {
               fprintf(stderr, "**** %s: signalled\n", script);
          } else {
               fprintf(stderr, "**** %s: exit status %d\n", script, WEXITSTATUS(res));
          }

          free(buffer);
     }
}

yacad_event_t *yacad_event_new_action(yacad_event_action action, yacad_event_callback callback, void *data) {
     yacad_event_impl_t *result = malloc(sizeof(yacad_event_impl_t));
     result->fn = impl_fn;
     result->action = action;
     result->callback = callback;
     result->action_data = result->callback_data = data;
     return &(result->fn);
}

yacad_event_t *yacad_event_new_conf(yacad_conf_t *conf, yacad_event_callback callback, void *data) {
     yacad_event_impl_t *result = malloc(sizeof(yacad_event_impl_t));

     sleep(1);

     result->fn = impl_fn;
     result->action = (yacad_event_action)script_action;
     result->callback = callback;
     result->action_data = conf;
     result->callback_data = data;

     return &(result->fn);
}
