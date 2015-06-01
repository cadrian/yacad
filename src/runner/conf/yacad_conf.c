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
#include "common/json/yacad_json_finder.h"

#define DEFAULT_ENDPOINT_PORT 1789
#define DEFAULT_EVENTS_PORT 1791
#define DEFAULT_WORK_PATH "."

static const char *dirs[] = {
     "/etc/xdg/yacad",
     "/etc/yacad",
     "/usr/local/etc/yacad",
     ".",
     NULL
};

typedef struct yacad_conf_impl_s {
     yacad_conf_t fn;
     yacad_runnerid_t *runnerid;
     json_value_t *json;
     int generation;
     char *filename;
     char *work_path;
     char *endpoint_name;
     char *events_name;
} yacad_conf_impl_t;

static yacad_runnerid_t *get_runnerid(yacad_conf_impl_t *this) {
     return this->runnerid;
}

static const char *get_endpoint_name(yacad_conf_impl_t *this) {
     return this->endpoint_name;
}

static const char *get_events_name(yacad_conf_impl_t *this) {
     return this->events_name;
}

static const char *get_work_path(yacad_conf_impl_t *this) {
     return this->work_path;
}

static int generation(yacad_conf_impl_t *this) {
     return this->generation;
}

static void free_(yacad_conf_impl_t *this) {
     if (this->json != NULL) {
          this->json->accept(this->json, json_kill());
     }
     this->runnerid->free(this->runnerid);
     free(this->events_name);
     free(this->endpoint_name);
     free(this->work_path);
     free(this->filename);
     free(this);
}

static yacad_conf_t impl_fn =  {
     .log = NULL,
     .get_runnerid = (yacad_conf_get_runnerid_fn)get_runnerid,
     .get_endpoint_name = (yacad_conf_get_endpoint_name_fn)get_endpoint_name,
     .get_events_name = (yacad_conf_get_events_name_fn)get_events_name,
     .get_work_path = (yacad_conf_get_work_path_fn)get_work_path,
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
     snprintf(filename, 4096, "%s/runner.conf", dir);
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
     yacad_json_finder_t *v = yacad_json_finder_new(I(this)->log, json_type_string, "logging/level");
     size_t i, n;
     char *level;
     json_string_t *jlevel;
     v->visit(v, this->json);
     jlevel = v->get_string(v);
     if (jlevel != NULL) {
          n = jlevel->count(jlevel) + 1;
          level = alloca(n);
          i = jlevel->utf8(jlevel, level, n);
          if (!strncmp("error", level, i)) {
               this->fn.log = get_logger(error);
          } else if (!strncmp("warn", level, i)) {
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

static void set_work_path(yacad_conf_impl_t *this) {
     yacad_json_finder_t *v = yacad_json_finder_new(I(this)->log, json_type_string, "%s");
     size_t n;
     json_string_t *jstring;

     v->visit(v, this->json, "work_path");
     jstring = v->get_string(v);
     if (jstring != NULL) {
          n = jstring->count(jstring) + 1;
          this->work_path = realloc(this->work_path, n);
          jstring->utf8(jstring, this->work_path, n);
          if (mkpath(this->work_path, 0700) != 0 && errno != EEXIST) {
               I(this)->log(warn, "Could not create directory: %s (%s)", this->work_path, strerror(errno));
          }
     } else {
          n = snprintf("", 0, "%s", DEFAULT_WORK_PATH) + 1;
          this->work_path = realloc(this->work_path, n);
          snprintf(this->work_path, n, "%s", DEFAULT_WORK_PATH);
     }

     v->visit(v, this->json, "core/endpoint");
     jstring = v->get_string(v);
     if (jstring != NULL) {
          n = jstring->count(jstring) + 1;
          this->endpoint_name = realloc(this->endpoint_name, n);
          jstring->utf8(jstring, this->endpoint_name, n);
     } else {
          n = snprintf("", 0, "tcp://*:%d", DEFAULT_ENDPOINT_PORT) + 1;
          this->endpoint_name = realloc(this->endpoint_name, n);
          snprintf(this->endpoint_name, n, "tcp://*:%d", DEFAULT_ENDPOINT_PORT);
     }

     v->visit(v, this->json, "core/events");
     jstring = v->get_string(v);
     if (jstring != NULL) {
          n = jstring->count(jstring) + 1;
          this->events_name = realloc(this->events_name, n);
          jstring->utf8(jstring, this->events_name, n);
     } else {
          n = snprintf("", 0, "tcp://*:%d", DEFAULT_EVENTS_PORT) + 1;
          this->events_name = realloc(this->events_name, n);
          snprintf(this->events_name, n, "tcp://*:%d", DEFAULT_EVENTS_PORT);
     }

     I(v)->free(I(v));
}

yacad_conf_t *yacad_runner_conf_new(void) {
     // The actual conf is read only once; after, the JSON object is shared.
     static yacad_conf_impl_t *ref = NULL;

     yacad_conf_impl_t *result = malloc(sizeof(yacad_conf_impl_t));
     int i;

     result->fn = impl_fn;
     result->fn.log = get_logger(debug);

     result->filename = result->work_path = result->endpoint_name = result->events_name = NULL;
     result->json = NULL;
     result->generation = 0;

     if (ref == NULL) {
          for (i = 0; dirs[i] != NULL && result->json == NULL; i++) {
               read_conf(result, dirs[i]);
          }
          if (result->json != NULL) {
               set_logger(result);
               I(result)->log(info, "Read configuration from %s", result->filename);
               set_work_path(result);
               I(result)->log(info, "Work path is %s", result->work_path);
               I(result)->log(info, "Core 0MQ endpoint is %s", result->endpoint_name);
               I(result)->log(info, "Core 0MQ events is %s", result->events_name);
          }
          ref = result;
     } else {
          result->json = ref->json;
     }

     return I(result);
}
