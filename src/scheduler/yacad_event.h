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

#ifndef __YACAD_EVENT_H__
#define __YACAD_EVENT_H__

#include "conf/yacad_conf.h"

typedef struct yacad_event_s yacad_event_t;

typedef int (*yacad_event_get_timeout_fn)(yacad_event_t *this);
typedef void (*yacad_event_run_fn)(yacad_event_t *this);
typedef void (*yacad_event_free_fn)(yacad_event_t *this);

struct yacad_event_s {
     yacad_event_get_timeout_fn getçtimeout;
     yacad_event_run_fn run;
     yacad_event_free_fn free;
};

yacad_event_t *yacad_event_new(yacad_conf_t *conf);

#endif /* __YACAD_EVENT_H__ */
