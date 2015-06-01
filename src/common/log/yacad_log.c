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

#include "yacad_log.h"
#include "common/zmq/yacad_zmq.h"

#define INPROC_ADDRESS "inproc://logger"

static int serr(const char *format, ...) {
     int result;
     va_list arg;
     va_start(arg, format);
     result = vfprintf(stderr, format, arg);
     va_end(arg);
     return result;
}

static logger_fn current_logger_fn = serr;

static bool_t do_log(yacad_zmq_poller_t *zpoller, yacad_zmq_socket_t *zscheduler, const char *message, void *data) {
     ((logger_fn)data)("%s\n", message);
     return true;
}

static void *logger_routine(logger_fn fn) {
     yacad_zmq_socket_t *zscheduler;
     yacad_zmq_poller_t *zpoller;

     zscheduler = yacad_zmq_socket_bind(NULL, INPROC_ADDRESS, ZMQ_PAIR);
     if (zscheduler != NULL) {
          set_thread_name("logger");

          zpoller = yacad_zmq_poller_new(NULL);
          if (zpoller != NULL) {
               zpoller->on_pollin(zpoller, zscheduler, do_log);
               zpoller->run(zpoller, fn);
               zpoller->free(zpoller);
          }
          zscheduler->free(zscheduler);
     }

     return NULL;
}

static void send_log(level_t level, const char *format, va_list arg) {
     yacad_zmq_socket_t *zlogger;
     struct timeval tm;
     char tag[256];
     static char *tagname[] = {
          "ERROR",
          "WARN ",
          "INFO ",
          "DEBUG",
          "TRACE",
     };
     char date[20];
     int t, n;
     va_list zarg;
     char *logmsg;

     zlogger = yacad_zmq_socket_connect(NULL, INPROC_ADDRESS, ZMQ_PAIR);
     if (zlogger != NULL) {
          gettimeofday(&tm, NULL);
          tag[255] = '\0';
          t = snprintf(tag, 255, "%s.%06ld [%s] {%s} ", datetime(tm.tv_sec, date), tm.tv_usec, tagname[level], get_thread_name());

          va_copy(zarg, arg);
          n = vsnprintf("", 0, format, zarg);
          va_end(zarg);

          logmsg = alloca(t + n + 1);
          t = snprintf(logmsg, t + 1, "%s", tag);
          va_copy(zarg, arg);
          n = vsnprintf(logmsg + t, n + 1, format, zarg);
          va_end(zarg);

          zlogger->send(zlogger, logmsg);

          zlogger->free(zlogger);
     }
}

#define DEFUN_LOGGER(__level) \
     static int __level##_logger(level_t level, const char *format, ...) { \
          int result = 0;                                               \
          va_list arg;                                                  \
          if (level <= __level) {                                       \
               va_start(arg, format);                                   \
               send_log(level, format, arg);                            \
               va_end(arg);                                             \
          }                                                             \
          return result;                                                \
     }

DEFUN_LOGGER(error)
DEFUN_LOGGER(warn)
DEFUN_LOGGER(info)
DEFUN_LOGGER(debug)
DEFUN_LOGGER(trace)

logger_t get_logger(level_t level) {
     static volatile bool_t init = false;
     static pthread_t logger;

     if (!init) {
          init = true;
          pthread_create(&logger, NULL, (void*(*)(void*))logger_routine, current_logger_fn);
     }

     switch(level) {
     case error: return error_logger;
     case warn:  return warn_logger;
     case info:  return info_logger;
     case debug: return debug_logger;
     case trace: return trace_logger;
     }
     return NULL;
}

void set_logger_fn(logger_fn fn) {
     current_logger_fn = fn;
}
