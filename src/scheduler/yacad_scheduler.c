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

#include <poll.h>
#include <cad_event_queue.h>

#include "yacad_scheduler.h"
#include "yacad_event.h"
#include "conf/yacad_conf.h"
#include "tasklist/yacad_tasklist.h"

typedef struct yacad_scheduler_impl_s {
     yacad_scheduler_t fn;
     yacad_conf_t *conf;
     yacad_tasklist_t *tasklist;
     cad_event_queue_t *event_queue;
     volatile bool_t stop;
} yacad_scheduler_impl_t;

static bool_t is_done(yacad_scheduler_impl_t *this) {
     return !this->event_queue->is_running(this->event_queue);
}

static void wait_and_run(yacad_scheduler_impl_t *this) {
     int fd = this->event_queue->get_fd(this->event_queue);
     struct pollfd r;
     bool_t done = false;
     yacad_event_t *event = NULL;
     r.fd = fd;
     r.events = POLLIN;
     while (event == NULL) {
          poll(&r, 1, this->conf->get_poll_granularity(this->conf));
          if (r.revents & POLLIN) {
               event = this->event_queue->pull(this->event_queue);
          }
     }
     if (event != NULL) {
          while (event->get_timeout(event) > 0) {
               poll(NULL, 0, event->get_timeout(event));
          }
          event->run(event);
     }
}

// will stop eventually
static void stop(yacad_scheduler_impl_t *this) {
     this->stop = true;
}

static void free_(yacad_scheduler_impl_t *this) {
     this->event_queue->free(this->event_queue);
     this->tasklist->free(this->tasklist);
     this->conf->free(this->conf);
     free(this);
}

static void do_stop(yacad_event_t *event, yacad_scheduler_impl_t *this) {
     while (this->event_queue->is_running(this->event_queue)) {
          this->event_queue->stop(this->event_queue);
     }
}

static yacad_event_t *event_provider(yacad_scheduler_impl_t *this) {
     // runs in the event queue thread
     if (this->stop) {
          return yacad_conf_new_action((yacad_event_action_fn)do_stop, 0, this);
     }
     return yacad_event_new_conf(this->conf);
}

yacad_scheduler_t *yacad_scheduler_new(void) {
     yacad_scheduler_impl_t *result = malloc(sizeof(yacad_scheduler_impl_t));
     result->conf = yacad_conf_new();
     result->tasklist = yacad_tasklist_new(result->conf->get_database_name(result->conf));
     result->event_queue = cad_new_event_queue_pthread(stdlib_memory, (provide_data_fn)event_provider,
                                                       result->conf->get_event_queue_capacity(result->conf));
     result->event_queue->start(result->event_queue, result);
     return &(result->fn);
}
