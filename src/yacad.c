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

#include "yacad.h"

#define DATETIME_FORMAT "%Y-%m-%d %H:%M:%S"

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

logger_t get_logger(level_t level) {
     switch(level) {
     case warn:  return warn_logger;
     case info:  return info_logger;
     case debug: return debug_logger;
     case trace: return trace_logger;
     }
     return NULL;
}

const char *datetime(time_t t, char *tmbuf) {
     char *result = NULL;
     struct tm tm;
     if (localtime_r(&t, &tm) != NULL) {
          strftime(result = tmbuf, 20, DATETIME_FORMAT, &tm);
     }
     return tmbuf;
}

int mkpath(const char *dir, mode_t mode) {
     /* http://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix */

     if (!dir) {
          errno = EINVAL;
          return -1;
     }

     if (strlen(dir) == 1 && (dir[0] == '/' || dir[0] == '.')) {
          return 0;
     }

     mkpath(dirname(strdupa(dir)), mode);
     return mkdir(dir, mode);
}

const char *yacad_version(void) {
     static char *version = NULL;
     int n = 0;
     if (version == NULL) {
          n = snprintf("", 0, "%d.%d.%d%s%s%s%s",
                       YACAD_VER_MAJOR, YACAD_VER_MINOR, YACAD_VER_PATCH,
                       *YACAD_VER_PRERELEASE ? "-" : "", YACAD_VER_PRERELEASE,
                       *YACAD_VER_METADATA ? "+" : "", YACAD_VER_METADATA);
          version = malloc(n + 1);
          snprintf(version, n + 1, "%d.%d.%d%s%s%s%s",
                   YACAD_VER_MAJOR, YACAD_VER_MINOR, YACAD_VER_PATCH,
                   *YACAD_VER_PRERELEASE ? "-" : "", YACAD_VER_PRERELEASE,
                   *YACAD_VER_METADATA ? "+" : "", YACAD_VER_METADATA);
     }
     return version;
}

static void *zmq_context = NULL;

void *get_zmq_context(logger_t log) {
     if (zmq_context == NULL) {
          log(debug, "Creating 0MQ context\n");
          zmq_context = zmq_ctx_new();
     }
     return zmq_context;
}

void del_zmq_context(logger_t log) {
     if (zmq_context != NULL) {
          log(debug, "Terminating 0MQ context\n");
          zmq_ctx_term(zmq_context);
          zmq_context = NULL;
     }
}
