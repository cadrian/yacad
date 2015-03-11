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

#include "yacad_conf.h"
#include "core/project/yacad_project.h"
#include "cron/yacad_cron.h"
#include "json/yacad_json_visitor.h"

static const char *dirs[] = {
     "/etc/xdg/yacad",
     "/etc/yacad",
     "/usr/local/etc/yacad",
     ".",
     NULL
};

typedef struct yacad_conf_impl_s {
     yacad_conf_t fn;
     cad_hash_t *projects;
     cad_hash_t *runners;
     json_value_t *json;
     int generation;
     char *filename;
     char *root_path;
} yacad_conf_impl_t;

static cad_hash_t *get_projects(yacad_conf_impl_t *this) {
     return this->projects;
}

static cad_hash_t *get_runners(yacad_conf_impl_t *this) {
     return this->runners;
}

static int generation(yacad_conf_impl_t *this) {
     return this->generation;
}

static void free_(yacad_conf_impl_t *this) {
     if (this->json != NULL) {
          this->json->accept(this->json, json_kill());
     }
     free(this->root_path);
     free(this->filename);
     free(this);
}

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

static yacad_task_t *next_task(yacad_conf_impl_t *this) {
     /* DEAD CODE -- to be moved */
     yacad_task_t *result = NULL;
     const char *script = "yacad_scheduler.sh";
     int res;
     int pid;
     int io[2];
     char *buffer;

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
               result = new_task(buffer);
          } else if (WIFSIGNALED(res)) {
               fprintf(stderr, "**** %s: signalled\n", script);
          } else {
               fprintf(stderr, "**** %s: exit status %d\n", script, WEXITSTATUS(res));
          }

          free(buffer);
     }

     return result;
}

static yacad_conf_t impl_fn =  {
     .log = NULL,
     .next_task = (yacad_conf_next_task_fn)next_task,
     .get_projects = (yacad_conf_get_projects_fn)get_projects,
     .get_runners = (yacad_conf_get_runners_fn)get_runners,
     .generation = (yacad_conf_generation_fn)generation,
     .free = (yacad_conf_free_fn)free_,
};

static void on_error(json_input_stream_t *stream, int line, int column, const char *format, ...) {
     va_list args;
     va_start(args, format);
     fprintf(stderr, "**** Syntax error line %d, column %d: ", line, column);
     vfprintf(stderr, format, args);
     fprintf(stderr, "\n");
     va_end(args);
     exit(1);
}

static void read_conf(yacad_conf_impl_t *this, const char *dir) {
     static char filename[4096];
     FILE *file;
     json_input_stream_t *stream;
     snprintf(filename, 4096, "%s/core.conf", dir);
     file = fopen(filename, "r");
     if (file != NULL) {
          stream = new_json_input_stream_from_file(file, stdlib_memory);
          this->json = json_parse(stream, on_error, stdlib_memory);
          fclose(file);
          stream->free(stream);
          if (this->json == NULL) {
               fprintf(stderr, "**** Invalid JSON: %s\n", filename);
          } else {
               this->filename = strdup(filename);
          }
     }
}

static void set_logger(yacad_conf_impl_t *this) {
     yacad_json_visitor_t *v = yacad_json_visitor_new(I(this)->log, json_type_string, "logging/level");
     size_t i, n;
     char *level;
     json_string_t *jlevel;
     v->visit(v, this->json);
     jlevel = v->get_string(v);
     if (jlevel != NULL) {
          n = jlevel->count(jlevel) + 1;
          level = alloca(n);
          i = jlevel->utf8(jlevel, level, n);
          if (!strncmp("warn", level, i)) {
               this->fn.log = get_logger(warn);
          } else if (!strncmp("info", level, i)) {
               this->fn.log = get_logger(info);
          } else if (!strncmp("debug", level, i)) {
               this->fn.log = get_logger(debug);
          } else if (!strncmp("trace", level, i)) {
               this->fn.log = get_logger(trace);
          } else {
               fprintf(stderr, "**** Unknown level: '%s' (ignored)\n", level);
          }
     }
     I(v)->free(I(v));
}

static void set_root_path(yacad_conf_impl_t *this) {
     yacad_json_visitor_t *v = yacad_json_visitor_new(I(this)->log, json_type_string, "core/root_path");
     size_t n;
     json_string_t *jlevel;
     v->visit(v, this->json);
     jlevel = v->get_string(v);
     if (jlevel != NULL) {
          n = jlevel->count(jlevel) + 1;
          this->root_path = malloc(n);
          jlevel->utf8(jlevel, this->root_path, n);
          if (mkpath(this->root_path, 0700)) {
               I(this)->log(warn, "Could not create directory: %s\n", this->root_path);
          }
     } else {
          this->root_path = strdup(".");
     }
     I(v)->free(I(v));
}

static char *json_to_string(yacad_json_visitor_t *v, json_value_t *value, ...) {
     va_list args;
     size_t n;
     json_string_t *string;
     char *result = NULL;
     va_start(args, value);
     v->vvisit(v, value, args);
     va_end(args);
     string = v->get_string(v);
     if (string != NULL) {
          n = string->utf8(string, "", 0) + 1;
          result = malloc(n);
          string->utf8(string, result, n);
     }
     return result;
}

static void read_projects(yacad_conf_impl_t *this) {
     yacad_json_visitor_t *vprojects = yacad_json_visitor_new(I(this)->log, json_type_array, "projects");
     yacad_json_visitor_t *vproject;
     yacad_json_visitor_t *vscm;
     yacad_json_visitor_t *vrunner;
     json_value_t *jproject;
     json_array_t *jprojects;
     json_object_t *jrunner;
     yacad_project_t *project;
     json_object_t *jscm;
     char *name, *root_path, *crondesc;
     yacad_scm_t *scm = NULL;
     yacad_cron_t *cron = NULL;
     int i, n, np;
     vprojects->visit(vprojects, this->json);
     jprojects = vprojects->get_array(vprojects);
     if (jprojects != NULL) {
          np = jprojects->count(jprojects);
          if (np > 0) {
               vproject = yacad_json_visitor_new(I(this)->log, json_type_string, "%s");
               vscm = yacad_json_visitor_new(I(this)->log, json_type_object, "scm");
               vrunner = yacad_json_visitor_new(I(this)->log, json_type_object, "runner");
               for (i = 0; i < np; i++) {
                    jproject = jprojects->get(jprojects, i);
                    name = json_to_string(vproject, jproject, "name");
                    if (name == NULL) {
                         I(this)->log(warn, "Project without name!\n");
                    } else if (this->projects->get(this->projects, name) != NULL) {
                         I(this)->log(warn, "Duplicate project name: \"%s\"\n", name);
                    } else {
                         scm = NULL;
                         cron = NULL;
                         root_path = NULL;
                         jrunner = NULL;

                         // Prepare root_path
                         n = snprintf("", 0, "%s/%s", this->root_path, name) + 1;
                         root_path = malloc(n);
                         snprintf(root_path, n, "%s/%s", this->root_path, name);

                         // Prepare scm
                         vscm->visit(vscm, jproject);
                         jscm = vscm->get_object(vscm);
                         if (jscm != NULL) {
                              scm = yacad_scm_new(I(this)->log, jscm, root_path);
                         }

                         // Prepare cron
                         crondesc = json_to_string(vproject, jproject, "cron");
                         if (crondesc == NULL) {
                              crondesc = strdup("* * * * *");
                         }
                         cron = yacad_cron_parse(I(this)->log, crondesc);

                         // Prepare jrunner
                         vrunner->visit(vrunner, jproject);
                         jrunner = vrunner->get_object(vrunner);

                         if (scm == NULL) {
                              I(this)->log(warn, "Project \"%s\": undefined scm\n", name);
                              if (cron != NULL) {
                                   cron->free(cron);
                              }
                         } else if (cron == NULL) {
                              I(this)->log(warn, "Project \"%s\": invalid cron\n", name);
                              scm->free(scm);
                         } else {
                              project = yacad_project_new(I(this)->log, name, scm, cron, jrunner);
                              if (project == NULL) {
                                   cron->free(cron);
                                   scm->free(scm);
                              } else {
                                   I(this)->log(info, "Adding project: %s [%s]\n", name, crondesc);
                                   this->projects->set(this->projects, name, project);
                              }
                         }

                         free(root_path);
                    }
                    free(name);
               }
               I(vrunner)->free(I(vrunner));
               I(vscm)->free(I(vscm));
               I(vproject)->free(I(vproject));
          }
     }
     I(vprojects)->free(I(vprojects));
}

yacad_conf_t *yacad_conf_new(void) {
     // The actual conf is read only once; after, the JSON object is shared.
     static yacad_conf_impl_t *ref = NULL;

     yacad_conf_impl_t *result = malloc(sizeof(yacad_conf_impl_t));
     int i;

     result->fn = impl_fn;
     result->fn.log = get_logger(debug);

     result->projects = cad_new_hash(stdlib_memory, cad_hash_strings);
     result->runners = cad_new_hash(stdlib_memory, cad_hash_strings);
     result->filename = result->root_path = NULL;
     result->json = NULL;
     result->generation = 0;

     if (ref == NULL) {
          for (i = 0; dirs[i] != NULL && result->json == NULL; i++) {
               read_conf(result, dirs[i]);
          }
          if (result->json != NULL) {
               set_logger(result);
               I(result)->log(info, "Read configuration from %s\n", result->filename);
               set_root_path(result);
               I(result)->log(info, "Projects root path is %s\n", result->root_path);
               read_projects(result);
          }
          ref = result;
     } else {
          result->json = ref->json;
     }

     return I(result);
}
