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
#define INPROC_ADDRESS "inproc://logger"

#undef zmqcheck
#define zmqcheck(zmqaction) __zmqcheck(NULL, (zmqaction), warn, #zmqaction, __FILE__, __LINE__)

static void *logger_routine(void *nul) {
     void *zcontext = get_zmq_context();
     void *zscheduler = zmq_socket(zcontext, ZMQ_REP);
     zmq_msg_t msg;
     int n, c = 0, r;
     char *logmsg = NULL;
     zmq_pollitem_t zitems[] = {
          {zscheduler, 0, ZMQ_POLLIN, 0},
     };
     bool_t running = true;

     set_thread_name("logger");

     zmqcheck(zmq_bind(zscheduler, INPROC_ADDRESS));
     do {
          zmqcheck(r = zmq_poll(zitems, 1, -1));
          if (zitems[0].revents & ZMQ_POLLIN) {
               zmqcheck(zmq_msg_init(&msg));
               zmqcheck(n = zmq_msg_recv(&msg, zscheduler, 0));
               if (n > 0) {
                    if (c < n) {
                         logmsg = realloc(logmsg, n);
                         c = n;
                    }
                    memcpy(logmsg, zmq_msg_data(&msg), n);
                    fprintf(stderr, "%s\n", logmsg);
               }
               zmqcheck(zmq_msg_close(&msg));
               zmqcheck(zmq_send(zscheduler, "", 0, 0));
          }
     } while (running);

     zmqcheck(zmq_close(zscheduler));
     free(logmsg);

     return nul;
}

static void send_log(level_t level, char *format, va_list arg) {
     void *zcontext = get_zmq_context();
     void *zlogger = zmq_socket(zcontext, ZMQ_REQ);
     struct timeval tm;
     char tag[256];
     static char *tagname[] = {
          "WARN ",
          "INFO ",
          "DEBUG",
          "TRACE",
     };
     char date[20];
     zmq_msg_t msg;
     int t, n;
     va_list zarg;
     char *logmsg;

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

     zmqcheck(zmq_connect(zlogger, INPROC_ADDRESS));

     zmqcheck(zmq_msg_init_size(&msg, t + n + 1));
     memcpy(zmq_msg_data(&msg), logmsg, t + n + 1);
     zmqcheck(zmq_msg_send(&msg, zlogger, 0));
     zmqcheck(zmq_msg_close(&msg));

     zmqcheck(zmq_recv(zlogger, "", 0, 0));

     zmqcheck(zmq_close(zlogger));
}

#define DEFUN_LOGGER(__level) \
     static int __level##_logger(level_t level, char *format, ...) {    \
          int result = 0;                                               \
          va_list arg;                                                  \
          if (level <= __level) {                                       \
               va_start(arg, format);                                   \
               send_log(level, format, arg);                            \
               va_end(arg);                                             \
          }                                                             \
          return result;                                                \
     }

DEFUN_LOGGER(warn)
DEFUN_LOGGER(info)
DEFUN_LOGGER(debug)
DEFUN_LOGGER(trace)

logger_t get_logger(level_t level) {
     static volatile bool_t init = false;
     static pthread_t logger;

     if (!init) {
          init = true;
          pthread_create(&logger, NULL, (void*(*)(void*))logger_routine, NULL);
     }

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

void *get_zmq_context(void) {
     if (zmq_context == NULL) {
          zmq_context = zmq_ctx_new();
          if (zmq_context == NULL) {
               fprintf(stderr, "**** ERROR: could not create 0MQ context\n");
               exit(1);
          }
     }
     return zmq_context;
}

void del_zmq_context(void) {
     if (zmq_context != NULL) {
          zmqcheck(zmq_ctx_term(zmq_context));
          zmq_context = NULL;
     }
}

static __thread const char *thread_name = NULL;

const char *get_thread_name(void) {
     return thread_name;
}

void set_thread_name(const char *tn) {
     thread_name = tn;
}

bool_t __zmqcheck(logger_t log, int zmqerr, level_t level, const char *zmqaction, const char *file, unsigned int line) {
     int result = true;
     char *paren;
     int len, err;
     if (zmqerr == -1) {
          err = zmq_errno();
          log(debug, "Error %d in %s line %u: %s", err, file, line, zmqaction);
          paren = strchrnul(zmqaction, '(');
          len = paren - zmqaction;
          if (log != NULL) {
               log(level, "%.*s: error: %s", len, zmqaction, zmq_strerror(err));
          } else {
               fprintf(stderr, "%.*s: error: %s", len, zmqaction, zmq_strerror(err));
               // no logger, don't continue
               exit(1);
          }
          result = false;
     }
     return result;
}
