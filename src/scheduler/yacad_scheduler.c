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
#include <unistd.h>
#include <time.h>

#include <cad_event_queue.h>

#include "yacad_scheduler.h"
#include "yacad_event.h"
#include "tasklist/yacad_tasklist.h"
#include "conf/yacad_project.h"

typedef enum {
     state_init = 0, state_running, state_stopping, state_stopped
} state_t;

typedef enum {
     check_state_ready = 0, check_state_checking, check_state_done
} check_state_t;

typedef struct {
     struct timeval time; // time of the next check
     check_state_t state; // to avoid double events
     int confgen;
} next_check_t;

typedef struct yacad_scheduler_impl_s {
     yacad_scheduler_t fn;
     yacad_conf_t *conf;
     yacad_tasklist_t *tasklist;
     cad_event_queue_t *event_queue;
     volatile state_t state;
     volatile next_check_t next_check;
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

typedef struct {
     yacad_scheduler_impl_t *this;
     yacad_events_t *events;
} synchronized_prepare_args_t;

static void synchronized_prepare(synchronized_prepare_args_t *args) {
     yacad_scheduler_impl_t *this = args->this;
     yacad_events_t *events = args->events;
     int fd;

     if (this->state < state_stopped) {
          fd = this->event_queue->get_fd(this->event_queue);
          events->on_read(events, (on_event_action)run, fd, this);
     }
}

static void prepare(yacad_scheduler_impl_t *this, yacad_events_t *events) {
     synchronized_prepare_args_t args = {this, events};
     this->event_queue->synchronized(this->event_queue, (synchronized_data_fn)synchronized_prepare, &args);
}

// will stop eventually
static void synchronized_stop(yacad_scheduler_impl_t *this) {
     if (this->state < state_stopping) {
          this->state = state_stopping;
     }
}

static void stop(yacad_scheduler_impl_t *this) {
     this->event_queue->synchronized(this->event_queue, (synchronized_data_fn)synchronized_stop, this);
}

static void free_(yacad_scheduler_impl_t *this) {
     this->event_queue->free(this->event_queue);
     this->tasklist->free(this->tasklist);
     this->conf->free(this->conf);
     free(this);
}

static void synchronized_do_stop(yacad_scheduler_impl_t *this) {
     this->state = state_stopped;
}

static void do_stop(yacad_event_t *event, yacad_scheduler_impl_t *this) {
     while (this->event_queue->is_running(this->event_queue)) {
          this->event_queue->stop(this->event_queue);
     }
     this->event_queue->synchronized(this->event_queue, (synchronized_data_fn)synchronized_do_stop, this);
}

static void iterate_next_check(cad_hash_t *tasklist_per_runner, int index, const char *key, yacad_project_t *project, yacad_scheduler_impl_t *this) {
     struct timeval tm = project->next_check(project);
     if (timercmp(&tm, &(this->next_check.time), <)) {
          this->next_check.time = tm;
     }
}

static void next_check(yacad_scheduler_impl_t *this) {
     cad_hash_t *projects = this->conf->get_projects(this->conf);
     struct timeval tm, save_tm = this->next_check.time;
     struct tm t;
     char tmbuf[64];

     gettimeofday(&tm, NULL);
     tm.tv_sec += 3600; // max one hour without a check
     tm.tv_usec = 0;
     this->next_check.time = tm;
     projects->iterate(projects, (cad_hash_iterator_fn)iterate_next_check, this);

     if (timercmp(&save_tm, &(this->next_check.time), !=) && this->conf->log(debug, "Next check time: ") > 0) {
          localtime_r((time_t*)&(this->next_check.time.tv_sec), &t);
          strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", &t);
          this->conf->log(debug, "%s\n", tmbuf);
     }
}

static void on_check_project(yacad_project_t *project, yacad_scheduler_impl_t *this) {
     // TODO add task because the check succeeded
     //this->tasklist->add(this->tasklist, task);
}

static void iterate_check_project(cad_hash_t *tasklist_per_runner, int index, const char *key, yacad_project_t *project, yacad_scheduler_impl_t *this) {
     project->check(project, (on_success_action)on_check_project, this);
}

static void synchronized_check_done(yacad_scheduler_impl_t *this) {
     this->next_check.state = check_state_done;
}

static void do_check(yacad_event_t *event, yacad_event_callback callback, yacad_scheduler_impl_t *this) {
     cad_hash_t *projects = this->conf->get_projects(this->conf);
     projects->iterate(projects, (cad_hash_iterator_fn)iterate_check_project, this);
     this->event_queue->synchronized(this->event_queue, (synchronized_data_fn)synchronized_check_done, this);
}

static yacad_event_t *event_provider(yacad_scheduler_impl_t *this) {
     // runs in the event queue thread, that's why I don't want to use this->conf - not sure if the conf will be stateless event though it should be

     yacad_event_t *result = NULL;
     int confgen;

     static yacad_conf_t *conf = NULL;
     struct timeval now;

     if (conf == NULL) {
          conf = yacad_conf_new();
     }

     switch(this->next_check.state) {
     case check_state_checking:
          // don't override yet, we wait for the project currently being checked to have finished.
          break;
     case check_state_ready:
          confgen = this->conf->generation(this->conf);
          if (confgen != this->next_check.confgen) {
               next_check(this);
               this->next_check.confgen = confgen;
          }
          break;
     case check_state_done:
          next_check(this);
          this->next_check.state = check_state_ready;
          break;
     }

     sleep(1); // roughly 1 event per second (no need to be accurate here; we just have to regularly check if we must stop)

     switch(this->state) {
     case state_running:
          if (this->next_check.state == check_state_ready) {
               gettimeofday(&now, NULL);
               if (timercmp(&(this->next_check.time), &now, <)) {
                    this->next_check.state = check_state_checking;
                    result = yacad_event_new_action((yacad_event_action)do_check, 0, this);
               }
          }
          break;
     case state_stopping:
          result = yacad_event_new_action((yacad_event_action)do_stop, 0, this);
          break;
     default:
          // ignored
          break;
     }

     return result;
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
     result->next_check.state = check_state_ready;
     result->next_check.confgen = -1;
     result->state = state_init;
     result->tasklist = yacad_tasklist_new(conf);
     result->event_queue = cad_new_event_queue_pthread(stdlib_memory, (provide_data_fn)event_provider, 16);
     result->state = state_running;
     result->event_queue->start(result->event_queue, result);
     return I(result);
}
