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

#define _GNU_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <alloca.h>

#include <cad_hash.h>
#include <json.h>

#include "yacad_conf.h"

static const char *dirs[] = {
     "/etc/xdg/yacad",
     "/etc/yacad",
     "/usr/local/etc/yacad",
     ".",
     NULL
};

typedef enum {
     json_type_object,
     json_type_array,
     json_type_string,
     json_type_number,
     json_type_const,
} json_type_t;

typedef struct yacad_conf_impl_s {
     yacad_conf_t fn;
     const char *database_name;
     const char *action_script;
     cad_hash_t *projects;
     cad_hash_t *runners;
     json_value_t *json;
} yacad_conf_impl_t;

static void taglog(level_t level) {
   struct timeval tm;
   char buffer[20];
   static char *tag[] = {
      "WARN ",
      "INFO ",
      "DEBUG"
   };
   gettimeofday(&tm, NULL);
   strftime(buffer, 20, "%Y/%m/%d %H:%M:%S", localtime(&tm.tv_sec));
   fprintf(stderr, "%s.%06ld [%s] ", buffer, tm.tv_usec, tag[level]);
}

static int warn_logger(level_t level, char *format, ...) {
   int result = 0;
   va_list arg;
   if(level <= warn) {
      va_start(arg, format);
      taglog(level);
      result = vfprintf(stderr, format, arg);
      va_end(arg);
   }
   return result;
}

static int info_logger(level_t level, char *format, ...) {
   int result = 0;
   va_list arg;
   if(level <= info) {
      va_start(arg, format);
      taglog(level);
      result = vfprintf(stderr, format, arg);
      va_end(arg);
   }
   return result;
}

static int debug_logger(level_t level, char *format, ...) {
   int result = 0;
   va_list arg;
   if(level <= debug) {
      va_start(arg, format);
      taglog(level);
      result = vfprintf(stderr, format, arg);
      va_end(arg);
   }
   return result;
}

static const char *get_database_name(yacad_conf_impl_t *this) {
     return this->database_name;
}

static yacad_project_t *get_project(yacad_conf_impl_t *this, const char *project_name) {
     return this->projects->get(this->projects, project_name);
}

static yacad_runner_t *get_runner(yacad_conf_impl_t *this, const char *runner_name) {
     return this->runners->get(this->runners, runner_name);
}

static void free_(yacad_conf_impl_t *this) {
     if (this->json != NULL) {
          this->json->accept(this->json, json_kill());
     }
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
     .get_project = (yacad_conf_get_project_fn)get_project,
     .get_runner = (yacad_conf_get_runner_fn)get_runner,
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
          printf("Reading configuration file %s\n", filename);
          stream = new_json_input_stream_from_file(file, stdlib_memory);
          this->json = json_parse(stream, on_error, stdlib_memory);
          fclose(file);
          stream->free(stream);
          if (this->json == NULL) {
               fprintf(stderr, "**** Invalid JSON: %s\n", filename);
          }
     }
}

typedef struct {
     json_visitor_t fn;
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
     char path[0];
} conf_visitor_t;

static void free_visitor(conf_visitor_t *this) {
     free(this);
}

static void visit_object(conf_visitor_t *this, json_object_t *visited) {
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
               value->accept(value, &(this->fn));
          }
     }
}

static void visit_array(conf_visitor_t *this, json_array_t *visited) {
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
               value->accept(value, &(this->fn));
          }
     }
}

static void visit_string(conf_visitor_t *this, json_string_t *visited) {
     this->value.string = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_string);
     }
}

static void visit_number(conf_visitor_t *this, json_number_t *visited) {
     this->value.number = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_number);
     }
}

static void visit_const(conf_visitor_t *this, json_const_t *visited) {
     this->value.constant = visited;
     if (this->current_path[0] == '\0') {
          this->found = (this->expected_type == json_type_const);
     }
}

static __attribute__((format(printf, 3, 4))) conf_visitor_t *conf_visitor(yacad_conf_impl_t *conf, json_type_t expected_type, const char *pathformat, ...) {
     static struct json_visitor visitor = {
          .free = (json_visit_free_fn)free_visitor,
          .visit_object = (json_visit_object_fn)visit_object,
          .visit_array = (json_visit_array_fn)visit_array,
          .visit_string = (json_visit_string_fn)visit_string,
          .visit_number = (json_visit_number_fn)visit_number,
          .visit_const = (json_visit_const_fn)visit_const,
     };
     conf_visitor_t *result;
     va_list args;
     int n;

     va_start(args, pathformat);
     n = vsnprintf("", 0, pathformat, args) + 1;
     va_end(args);

     result = malloc(sizeof(conf_visitor_t) + n);
     va_start(args, pathformat);
     vsnprintf(result->path, n, pathformat, args);
     va_end(args);

     result->fn = visitor;
     result->conf = conf;
     result->found = false;
     result->expected_type = expected_type;
     result->current_path = result->path;
     return result;
}

static void set_logger(yacad_conf_impl_t *this) {
     conf_visitor_t *v = conf_visitor(this, json_type_string, "debug/level");
     size_t i, n;
     char *level;
     json_string_t *jlevel;
     this->json->accept(this->json, &(v->fn));
     if (v->found) {
          jlevel = v->value.string;
          n = jlevel->count(jlevel);
          level = alloca(n+1);
          i = jlevel->utf8(jlevel, level, n+1);
          if (!strncmp("debug", level, i)) {
               this->fn.log = debug_logger;
          } else if (!strncmp("info", level, i)) {
               this->fn.log = info_logger;
          } else if (!strncmp("warn", level, i)) {
               this->fn.log = warn_logger;
          } else {
               fprintf(stderr, "**** Unknown level: '%s' (ignored)\n", level);
          }
     }
     v->fn.free(&(v->fn));
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
     result->json = NULL;

     if (ref == NULL) {
          for (i = 0; dirs[i] != NULL && result->json == NULL; i++) {
               read_conf(result, dirs[i]);
          }
          if (result->json != NULL) {
               set_logger(result);
          }
          ref = result;
     } else {
          result->json = ref->json;
     }

     return &(result->fn);
}
