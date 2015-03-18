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

#include "yacad_scheduler.h"
#include "core/project/yacad_project.h"
#include "core/tasklist/yacad_tasklist.h"

#define INPROC_ADDRESS "inproc://scheduler"
#define MSG_START "start"
#define MSG_CHECK "check"

typedef struct {
     struct timeval time; // time of the next check
     int confgen;
} next_check_t;

typedef struct yacad_scheduler_impl_s {
     yacad_scheduler_t fn;
     yacad_conf_t *conf;
     yacad_tasklist_t *tasklist;
     next_check_t worker_next_check;
     pthread_t worker;
} yacad_scheduler_impl_t;

static bool_t is_done(yacad_scheduler_impl_t *this) {
     return false; // TODO
}

static void stop(yacad_scheduler_impl_t *this) {
     // TODO
}

static void free_(yacad_scheduler_impl_t *this) {
     this->tasklist->free(this->tasklist);
     this->conf->free(this->conf);
     free(this);
}

static void iterate_next_check(cad_hash_t *projects, int index, const char *key, yacad_project_t *project, yacad_scheduler_impl_t *this) {
     struct timeval tm = project->next_check(project);
     if (this->worker_next_check.time.tv_sec == 0 || timercmp(&tm, &(this->worker_next_check.time), <)) {
          this->worker_next_check.time = tm;
     }
}

static void worker_next_check(yacad_scheduler_impl_t *this) {
     cad_hash_t *projects = this->conf->get_projects(this->conf);
     struct timeval tmzero = {0,0}, save_tm = this->worker_next_check.time;
     char tmbuf[20];

     this->worker_next_check.time = tmzero;
     projects->iterate(projects, (cad_hash_iterator_fn)iterate_next_check, this);

     if (timercmp(&save_tm, &(this->worker_next_check.time), !=)) {
          this->conf->log(debug, "Next check time: %s", datetime(this->worker_next_check.time.tv_sec, tmbuf));
     }
}

static void *worker_routine(yacad_scheduler_impl_t *this) {
     void *zcontext = get_zmq_context();
     void *zscheduler = zmq_socket(zcontext, ZMQ_PAIR);
     int confgen;
     bool_t running = false;
     bool_t check = false;
     int i;
     char buffer[16];
     size_t n = sizeof(buffer);
     struct timeval now;
     zmq_pollitem_t zitems[] = {
          {zscheduler, 0, ZMQ_POLLIN, 0},
     };
     long timeout;

     zmq_connect(zscheduler, INPROC_ADDRESS); // TODO error checking

     do {
          i = zmq_recv(zscheduler, buffer, n, 0); // TODO error checking
          if (!strncmp(buffer, MSG_START, i)) {
               running = true;
          }
     } while (!running);

     while (running) {
          confgen = this->conf->generation(this->conf);
          if (check || confgen != this->worker_next_check.confgen) {
               worker_next_check(this);
               this->worker_next_check.confgen = confgen;
               check = false;
          }

          gettimeofday(&now, NULL);
          timeout = this->worker_next_check.time.tv_sec - now.tv_sec;
          if (timeout < 0) {
               timeout = 0;
          }

          zmq_poll(zitems, 1, timeout * 1000L);
          if (zitems[0].revents & ZMQ_POLLIN) {
               // TODO: stop running?
          } else {
               gettimeofday(&now, NULL);
               if (timercmp(&(this->worker_next_check.time), &now, <)) {
                    zmq_send(zscheduler, MSG_CHECK, strlen(MSG_CHECK), 0); // TODO error checking
                    check = true;
               }
          }
     }

     zmq_close(zscheduler);

     return this;
}

static void iterate_check_project(cad_hash_t *projects, int index, const char *project_name, yacad_project_t *project, yacad_scheduler_impl_t *this) {
     yacad_task_t *task = project->check(project);
     if (task != NULL) {
          this->tasklist->add(this->tasklist, task);
     }
}

static void run(yacad_scheduler_impl_t *this) {
     void *zcontext = get_zmq_context();
     void *zworker = zmq_socket(zcontext, ZMQ_PAIR);
     void *zrunner = zmq_socket(zcontext, ZMQ_REP);
     bool_t done = false;
     int i;
     char buffer[16];
     size_t n = sizeof(buffer);
     cad_hash_t *projects;
     zmq_pollitem_t zitems[] = {
          {zworker, 0, ZMQ_POLLIN, 0},
          {zrunner, 0, ZMQ_POLLIN, 0},
     };

     zmq_bind(zworker, INPROC_ADDRESS); // TODO error checking

     zmq_send(zworker, MSG_START, strlen(MSG_START), 0); // TODO error checking

     do {
          zmq_poll(zitems, 2, -1);
          if (zitems[0].revents & ZMQ_POLLIN) {
               i = zmq_recv(zworker, buffer, n, 0); // TODO error checking
               if (!strncmp(buffer, MSG_CHECK, i)) {
                    this->conf->log(debug, "Checking projects");
                    projects = this->conf->get_projects(this->conf);
                    projects->iterate(projects, (cad_hash_iterator_fn)iterate_check_project, this);
               }
          } else if (zitems[1].revents & ZMQ_POLLIN) {
               i = zmq_recv(zrunner, buffer, n, 0); // TODO error checking
               // TODO send a task according to the runner description
          }
     } while (!done);

     zmq_close(zworker);
}

static yacad_scheduler_t impl_fn = {
     .is_done = (yacad_scheduler_is_done_fn)is_done,
     .run = (yacad_scheduler_run_fn)run,
     .stop = (yacad_scheduler_stop_fn)stop,
     .free = (yacad_scheduler_free_fn)free_,
};

yacad_scheduler_t *yacad_scheduler_new(yacad_conf_t *conf) {
     yacad_scheduler_impl_t *result = malloc(sizeof(yacad_scheduler_impl_t));
     struct timeval now;
     gettimeofday(&now, NULL);
     result->fn = impl_fn;
     result->conf = conf;
     result->worker_next_check.confgen = -1;
     result->worker_next_check.time = now;
     result->tasklist = yacad_tasklist_new(conf->log, conf->get_database_name(conf));
     pthread_create(&(result->worker), NULL, (void*(*)(void*))worker_routine, result);
     return I(result);
}
