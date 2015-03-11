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

#ifndef __YACAD_H__
#define __YACAD_H__

#include <alloca.h>
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cad_array.h>
#include <cad_event_queue.h>
#include <cad_events.h>
#include <cad_hash.h>
#include <cad_shared.h>

#include <json.h>

/* http://semver.org/ */
#define YACAD_VER_MAJOR 0
#define YACAD_VER_MINOR 0
#define YACAD_VER_PATCH 1
#define YACAD_VER_PRERELEASE ""
#define YACAD_VER_METADATA ""

#define I(impl) (&((impl)->fn))

typedef enum {
     false=0,
     true
} bool_t;

#define bool2str(flag) ((flag) ? "true" : "false")

/**
 * The log levels
 */
typedef enum {
     /**
      * Warnings (default level)
      */
     warn=0,
     /**
      * Informations on how ExP runs
      */
     info,
     /**
      * Developer details, usually not useful
      */
     debug,
     /**
      * Developper useless details
      */
     trace,
} level_t;

/**
 * A logger is a `printf`-like function to call
 *
 * @param[in] level the logging level
 * @param[in] format the message format
 * @param[in] ... the message arguments
 *
 * @return the length of the message after expansion; 0 if the log is not emitted because of the level
 */
typedef int (*logger_t) (level_t level, char *format, ...) __PRINTF__;

const char *yacad_version(void);
const char *datetime(time_t t, char *tmbuf);
int mkpath(const char *dir, mode_t mode);

logger_t get_logger(level_t level);

#endif /* __YACAD_H__ */
