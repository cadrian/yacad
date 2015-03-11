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

#ifndef __YACAD_SCHEDULER_H__
#define __YACAD_SCHEDULER_H__

#include "yacad.h"
#include "core/conf/yacad_conf.h"
#include "yacad_events.h"

typedef struct yacad_scheduler_s yacad_scheduler_t;

typedef bool_t (*yacad_scheduler_is_done_fn)(yacad_scheduler_t *this);
typedef void (*yacad_scheduler_prepare_fn)(yacad_scheduler_t *this, yacad_events_t *events);
typedef void (*yacad_scheduler_stop_fn)(yacad_scheduler_t *this);
typedef void (*yacad_scheduler_free_fn)(yacad_scheduler_t *this);

struct yacad_scheduler_s {
     yacad_scheduler_is_done_fn is_done;
     yacad_scheduler_prepare_fn prepare;
     yacad_scheduler_stop_fn stop;
     yacad_scheduler_free_fn free;
};

yacad_scheduler_t *yacad_scheduler_new(yacad_conf_t *conf);

#endif /* __YACAD_SCHEDULER_H__ */
