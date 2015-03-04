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

#include <stdio.h>

#include <cad_event_queue.h>

#include "yacad_scheduler.h"
#include "yacad_event.h"
#include "tasklist/yacad_tasklist.h"

#define STATE_INIT     0
#define STATE_RUNNING  1
#define STATE_STOPPING 2
#define STATE_STOPPED  3

typedef struct yacad_scheduler_impl_s {
     yacad_scheduler_t fn;
     yacad_conf_t *conf;
     yacad_tasklist_t *tasklist;
     cad_event_queue_t *event_queue;
     volatile int state;
} yacad_scheduler_impl_t;

static bool_t is_done(yacad_scheduler_impl_t *this) {
     return !this->event_queue->is_running(this->event_queue);
}

static void run(int fd, yacad_scheduler_impl_t *this) {
     yacad_event_t *event;
     if (fd == this->event_queue->get_fd(this->event_queue)) {
          event = this->event_queue->pull(this->event_queue);
          if (event != NULL) {
               event->run(event);
          }
     }
}

static void prepare(yacad_scheduler_impl_t *this, yacad_events_t *events) {
     int fd;
     if (this->state < STATE_STOPPED) {
          fd = this->event_queue->get_fd(this->event_queue);
          events->on_read(events, (on_event_action)run, fd, this);
     }
}

// will stop eventually
static void stop(yacad_scheduler_impl_t *this) {
     if (this->state < STATE_STOPPING) {
          this->state = STATE_STOPPING;
     }
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
     this->state = STATE_STOPPED;
}

static yacad_event_t *event_provider(yacad_scheduler_impl_t *this) {
     // runs in the event queue thread
     switch(this->state) {
     case STATE_RUNNING:
          return yacad_event_new_conf(this->conf);
     case STATE_STOPPING:
          return yacad_event_new_action((yacad_event_action)do_stop, 0, this);
     default:
          return NULL;
     }
}

static yacad_scheduler_t impl_fn = {
     .is_done = (yacad_scheduler_is_done_fn)is_done,
     .prepare = (yacad_scheduler_prepare_fn)prepare,
     .stop = (yacad_scheduler_stop_fn)stop,
     .free = (yacad_scheduler_free_fn)free_,
};

yacad_scheduler_t *yacad_scheduler_new(yacad_conf_t *conf) {
     yacad_scheduler_impl_t *result = malloc(sizeof(yacad_scheduler_impl_t));
     result->fn = impl_fn;
     result->conf = conf;
     result->state = STATE_INIT;
     result->tasklist = yacad_tasklist_new(result->conf->get_database_name(conf));
     result->event_queue = cad_new_event_queue_pthread(stdlib_memory, (provide_data_fn)event_provider, 16);
     result->event_queue->start(result->event_queue, result);
     result->state = STATE_RUNNING;
     return &(result->fn);
}
