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
#include <stdarg.h>
#include <alloca.h>
#include <time.h>

#include <cad_hash.h>

#include "yacad_conf.h"
#include "yacad_project.h"
#include "yacad_cron.h"

static const char *dirs[] = {
     "/etc/xdg/yacad",
     "/etc/yacad",
     "/usr/local/etc/yacad",
     ".",
     NULL
};

typedef struct yacad_conf_impl_s {
     yacad_conf_t fn;
     const char *database_name;
     const char *action_script;
     cad_hash_t *projects;
     cad_hash_t *runners;
     json_value_t *json;
     int generation;
     char *filename;
     char *root_path;
} yacad_conf_impl_t;

typedef struct {
     yacad_conf_visitor_t fn;
     yacad_conf_impl_t *conf;
     char *current_path;
     json_type_t expected_type;
     bool_t found;
     union {
          json_object_t *object;
          json_array_t *array;
          json_string_t *string;
          json_number_t *number;
          json_const_t *constant;
     } value;
     const char *pathformat;
} yacad_conf_visitor_impl_t;

static void taglog(level_t level) {
     struct timeval tm;
     char buffer[20];
     static char *tag[] = {
          "WARN ",
          "INFO ",
          "DEBUG",
          "TRACE",
     };
     gettimeofday(&tm, NULL);
     fprintf(stderr, "%s.%06ld [%s] ", datetime(tm.tv_sec, buffer), tm.tv_usec, tag[level]);
}

static int warn_logger(level_t level, char *format, ...) {
     static bool_t nl = true;
     int result = 0;
     va_list arg;
     if(level <= warn) {
          va_start(arg, format);
          if (nl) {
               taglog(level);
          }
          result = vfprintf(stderr, format, arg);
          va_end(arg);
          nl = format[strlen(format)-1] == '\n';
     }
     return result;
}

static int info_logger(level_t level, char *format, ...) {
     static bool_t nl = true;
     int result = 0;
     va_list arg;
     if(level <= info) {
          va_start(arg, format);
          if (nl) {
               taglog(level);
          }
          result = vfprintf(stderr, format, arg);
          va_end(arg);
          nl = format[strlen(format)-1] == '\n';
     }
     return result;
}

static int debug_logger(level_t level, char *format, ...) {
     static bool_t nl = true;
     int result = 0;
     va_list arg;
     if(level <= debug) {
          va_start(arg, format);
          if (nl) {
               taglog(level);
          }
          result = vfprintf(stderr, format, arg);
          va_end(arg);
          nl = format[strlen(format)-1] == '\n';
     }
     return result;
}

static int trace_logger(level_t level, char *format, ...) {
     static bool_t nl = true;
     int result = 0;
     va_list arg;
     if(level <= trace) {
          va_start(arg, format);
          if (nl) {
               taglog(level);
          }
          result = vfprintf(stderr, format, arg);
          va_end(arg);
          nl = format[strlen(format)-1] == '\n';
     }
     return result;
}

static const char *get_database_name(yacad_conf_impl_t *this) {
     return this->database_name;
}

static cad_hash_t *get_projects(yacad_conf_impl_t *this) {
     return this->projects;
}

static cad_hash_t *get_runners(yacad_conf_impl_t *this) {
     return this->runners;
}

static int generation(yacad_conf_impl_t *this) {
     return this->generation;
}

static yacad_conf_visitor_impl_t *conf_visitor(yacad_conf_impl_t *this, json_type_t expected_type, const char *pathformat);

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
     yacad_task_t *result = NULL;
     const char *script = this->action_script;
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
     .log = debug_logger,
     .get_database_name = (yacad_conf_get_database_name_fn)get_database_name,
     .next_task = (yacad_conf_next_task_fn)next_task,
     .get_projects = (yacad_conf_get_projects_fn)get_projects,
     .get_runners = (yacad_conf_get_runners_fn)get_runners,
     .generation = (yacad_conf_generation_fn)generation,
     .visitor = (yacad_conf_visitor_fn)conf_visitor,
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static json_object_t *get_object(yacad_conf_visitor_impl_t *this) {
     return this->found ? this->value.object : NULL;
}

static json_array_t *get_array(yacad_conf_visitor_impl_t *this) {
     return this->found ? this->value.array : NULL;
}

static json_string_t *get_string(yacad_conf_visitor_impl_t *this) {
     return this->found ? this->value.string : NULL;
}

static json_number_t *get_number(yacad_conf_visitor_impl_t *this) {
     return this->found ? this->value.number : NULL;
}

static json_const_t *get_const(yacad_conf_visitor_impl_t *this) {
     return this->found ? this->value.constant : NULL;
}

static void free_visitor(yacad_conf_visitor_impl_t *this) {
     free(this);
}

static void visit_object(yacad_conf_visitor_impl_t *this, json_object_t *visited) {
     char *key = this->current_path;
     char *next_path;
     json_value_t *value;
     this->value.object = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_object);
     } else {
          next_path = strchrnul(key, '/');
          this->current_path = next_path + (*next_path ? 1 : 0);
          *next_path = '\0';
          value = visited->get(visited, key);
          if (value != NULL) {
               value->accept(value, I(I(this)));
          }
     }
}

static void visit_array(yacad_conf_visitor_impl_t *this, json_array_t *visited) {
     char *key = this->current_path;
     char *next_path;
     json_value_t *value;
     int index;
     this->value.array = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_array);
     } else {
          next_path = strchrnul(key, '/');
          this->current_path = next_path + (*next_path ? 1 : 0);
          *next_path = '\0';
          index = atoi(key);
          value = visited->get(visited, index);
          if (value != NULL) {
               value->accept(value, I(I(this)));
          }
     }
}

static void visit_string(yacad_conf_visitor_impl_t *this, json_string_t *visited) {
     this->value.string = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_string);
     }
}

static void visit_number(yacad_conf_visitor_impl_t *this, json_number_t *visited) {
     this->value.number = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_number);
     }
}

static void visit_const(yacad_conf_visitor_impl_t *this, json_const_t *visited) {
     this->value.constant = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_const);
     }
}

static void vvisit(yacad_conf_visitor_impl_t *this, json_value_t *value, va_list vargs) {
     va_list args;
     int n;
     char *path;

     va_copy(args, vargs);
     n = vsnprintf("", 0, this->pathformat, args) + 1;
     va_end(args);

     path = malloc(n);

     va_copy(args, vargs);
     vsnprintf(path, n, this->pathformat, args);
     va_end(args);

     this->current_path = path;
     value->accept(value, I(I(this)));
     free(path);
}

static void visit(yacad_conf_visitor_impl_t *this, json_value_t *value, ...) {
     va_list args;
     va_start(args, value);
     vvisit(this, value, args);
     va_end(args);
}

static yacad_conf_visitor_t conf_visitor_fn = {
     .fn = {
          .free = (json_visit_free_fn)free_visitor,
          .visit_object = (json_visit_object_fn)visit_object,
          .visit_array = (json_visit_array_fn)visit_array,
          .visit_string = (json_visit_string_fn)visit_string,
          .visit_number = (json_visit_number_fn)visit_number,
          .visit_const = (json_visit_const_fn)visit_const,
     },
     .get_object = (yacad_conf_visitor_get_object_fn)get_object,
     .get_array = (yacad_conf_visitor_get_array_fn)get_array,
     .get_string = (yacad_conf_visitor_get_string_fn)get_string,
     .get_number = (yacad_conf_visitor_get_number_fn)get_number,
     .get_const = (yacad_conf_visitor_get_const_fn)get_const,
     .vvisit = (yacad_conf_visitor_vvisit_fn)vvisit,
     .visit = (yacad_conf_visitor_visit_fn)visit,
};

static yacad_conf_visitor_impl_t *conf_visitor(yacad_conf_impl_t *this, json_type_t expected_type, const char *pathformat) {
     yacad_conf_visitor_impl_t *result = malloc(sizeof(yacad_conf_visitor_impl_t));
     result->fn = conf_visitor_fn;
     result->conf = this;
     result->found = false;
     result->expected_type = expected_type;
     result->pathformat = pathformat;
     result->value.object = NULL;
     return result;
}

static void set_logger(yacad_conf_impl_t *this) {
     yacad_conf_visitor_impl_t *v = conf_visitor(this, json_type_string, "logging/level");
     size_t i, n;
     char *level;
     json_string_t *jlevel;
     visit(v, this->json);
     if (v->found) {
          jlevel = v->value.string;
          n = jlevel->count(jlevel) + 1;
          level = alloca(n);
          i = jlevel->utf8(jlevel, level, n);
          if (!strncmp("warn", level, i)) {
               this->fn.log = warn_logger;
          } else if (!strncmp("info", level, i)) {
               this->fn.log = info_logger;
          } else if (!strncmp("debug", level, i)) {
               this->fn.log = debug_logger;
          } else if (!strncmp("trace", level, i)) {
               this->fn.log = trace_logger;
          } else {
               fprintf(stderr, "**** Unknown level: '%s' (ignored)\n", level);
          }
     }
     I(I(v))->free(I(I(v)));
}

static void set_root_path(yacad_conf_impl_t *this) {
     yacad_conf_visitor_impl_t *v = conf_visitor(this, json_type_string, "core/root_path");
     size_t n;
     json_string_t *jlevel;
     visit(v, this->json);
     if (v->found) {
          jlevel = v->value.string;
          n = jlevel->count(jlevel) + 1;
          this->root_path = malloc(n);
          jlevel->utf8(jlevel, this->root_path, n);
     } else {
          this->root_path = strdup(".");
     }
     I(I(v))->free(I(I(v)));
}

static char *json_to_string(yacad_conf_visitor_impl_t *v, json_value_t *value, ...) {
     va_list args;
     size_t n;
     json_string_t *string;
     char *result = NULL;
     va_start(args, value);
     vvisit(v, value, args);
     va_end(args);
     if (v->found) {
          string = v->value.string;
          n = string->utf8(string, "", 0) + 1;
          result = malloc(n);
          string->utf8(string, result, n);
     }
     return result;
}

static void read_projects(yacad_conf_impl_t *this) {
     yacad_conf_visitor_impl_t *v = conf_visitor(this, json_type_array, "projects");
     yacad_conf_visitor_impl_t *p;
     yacad_conf_visitor_impl_t *s;
     json_array_t *projects;
     json_value_t *value;
     yacad_project_t *project;
     json_object_t *jscm;
     char *name, *root_path, *crondesc;
     yacad_scm_t *scm = NULL;
     yacad_cron_t *cron = NULL;
     int i, n;
     visit(v, this->json);
     if (v->found) {
          projects = v->value.array;
          n = projects->count(projects);
          p = conf_visitor(this, json_type_string, "%s");
          s = conf_visitor(this, json_type_object, "%s");
          for (i = 0; i < n; i++) {
               value = projects->get(projects, i);
               name = json_to_string(p, value, "name");
               if (name == NULL) {
                    I(this)->log(warn, "Project without name!\n");
               } else if (this->projects->get(this->projects, name) != NULL) {
                    I(this)->log(warn, "Duplicate project name: \"%s\"\n", name);
               } else {
                    scm = NULL;
                    cron = NULL;
                    root_path = NULL;

                    // Prepare root_path
                    n = snprintf("", 0, "%s/%s", this->root_path, name) + 1;
                    root_path = malloc(n);
                    snprintf(root_path, n, "%s/%s", this->root_path, name);

                    // Prepare scm
                    visit(s, value, "scm");
                    jscm = s->value.object;
                    if (jscm != NULL) {
                         scm = yacad_scm_new(I(this), jscm, root_path);
                    }

                    // Prepare cron
                    crondesc = json_to_string(p, value, "cron");
                    if (crondesc == NULL) {
                         crondesc = strdup("* * * * *");
                    }
                    cron = yacad_cron_parse(I(this), crondesc);

                    if (scm == NULL) {
                         I(this)->log(warn, "Project \"%s\": undefined scm\n", name);
                         free(cron);
                         scm->free(scm);
                    } else if (cron == NULL) {
                         I(this)->log(warn, "Project \"%s\": invalid cron\n", name);
                         free(cron);
                         scm->free(scm);
                    } else {
                         project = yacad_project_new(I(this), name, scm, cron);
                         if (project == NULL) {
                              free(cron);
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
          I(I(s))->free(I(I(s)));
          I(I(p))->free(I(I(p)));
     }
     I(I(v))->free(I(I(v)));
}

yacad_conf_t *yacad_conf_new(void) {
     // The actual conf is read only once; after, the JSON object is shared.
     static yacad_conf_impl_t *ref = NULL;

     yacad_conf_impl_t *result = malloc(sizeof(yacad_conf_impl_t));
     int i;

     result->fn = impl_fn;
     result->database_name = "yacad.db";
     result->action_script = "yacad_scheduler.sh";
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
