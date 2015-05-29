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

#ifndef __YACAD_CRON_H__
#define __YACAD_CRON_H__

#include "yacad.h"

typedef struct yacad_cron_s yacad_cron_t;

typedef struct timeval (*yacad_cron_next_fn)(yacad_cron_t *this);
typedef void (*yacad_cron_free_fn)(yacad_cron_t *this);

typedef struct tm (*get_current_minute_fn)(void);

struct yacad_cron_s {
     yacad_cron_next_fn next;
     yacad_cron_free_fn free;
};

yacad_cron_t *yacad_cron_parse(logger_t log, const char *cronspec, get_current_minute_fn gcm);

extern get_current_minute_fn default_get_current_minute;

#endif /* __YACAD_CRON_H__ */
