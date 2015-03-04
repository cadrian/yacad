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

#include <stdlib.h>

#include "yacad_event.h"

typedef struct yacad_event_impl_s {
     yacad_event_t fn;
     yacad_event_action_fn action;
     int timeout;
     void *data;
} yacad_event_impl_t;

static int get_timeout(yacad_event_impl_t*this) {
     return this->timeout;
}

static void run(yacad_event_impl_t *this) {
     this->action(&(this->fn), this->data);
}

static void free_(yacad_event_impl_t *this) {
     free(this);
}

static yacad_event_t impl_fn = {
     .get_timeout = (yacad_event_get_timeout_fn)get_timeout,
     .run = (yacad_event_run_fn)run,
     .free = (yacad_event_free_fn)free_,
};

yacad_event_t *yacad_event_new_action(yacad_event_action_fn action, int timeout, void *data) {
     yacad_event_impl_t *result = malloc(sizeof(yacad_event_impl_t));
     result->fn = impl_fn;
     result->action = action;
     result->timeout = timeout;
     result->data = data;
     return &(result->fn);
}

yacad_event_t *yacad_event_new_conf(yacad_conf_t *conf) {
     // TODO
     return NULL;
}
