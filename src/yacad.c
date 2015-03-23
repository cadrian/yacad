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
          zmqcheck(NULL, zmq_ctx_term(zmq_context), error);
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
          result = false;
          if (log != NULL) {
               err = zmq_errno();
               paren = strchrnul(zmqaction, '(');
               len = paren - zmqaction;
               log(debug, "Error %d in %s line %u: %s", err, file, line, zmqaction);
               log(level, "%.*s: error: %s", len, zmqaction, zmq_strerror(err));
          }
     }
     return result;
}
