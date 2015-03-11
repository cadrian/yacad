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

#ifndef __YACAD_EVENTS_H__
#define __YACAD_EVENTS_H__

#include "yacad.h"

typedef struct yacad_events_s yacad_events_t;

typedef void (*on_timeout_action)(void *data);
typedef void (*on_event_action)(int fd, void *data);

typedef int (*yacad_events_on_timeout_fn)(yacad_events_t *this, on_timeout_action action, void *data);
typedef int (*yacad_events_on_read_fn)(yacad_events_t *this, on_event_action action, int fd, void *data);
typedef int (*yacad_events_on_write_fn)(yacad_events_t *this, on_event_action action, int fd, void *data);
typedef int (*yacad_events_on_exception_fn)(yacad_events_t *this, on_event_action action, int fd, void *data);
typedef bool_t (*yacad_events_step_fn)(yacad_events_t *this);
typedef void (*yacad_events_free_fn)(yacad_events_t *this);

struct yacad_events_s {
     yacad_events_on_timeout_fn on_timeout;
     yacad_events_on_read_fn on_read;
     yacad_events_on_write_fn on_write;
     yacad_events_on_exception_fn on_exception;
     yacad_events_step_fn step;
     yacad_events_free_fn free;
};

yacad_events_t *yacad_events_new(void);

#endif /* __YACAD_EVENTS_H__ */
