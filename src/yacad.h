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
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <zmq.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cad_array.h>
#include <cad_hash.h>
#include <cad_shared.h>

#include <json.h>

#include "common/log/yacad_log.h"

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

const char *yacad_version(void);
const char *datetime(time_t t, char *tmbuf);
int mkpath(const char *dir, mode_t mode);

const char *get_thread_name(void);
void set_thread_name(const char *tn);

#endif /* __YACAD_H__ */
