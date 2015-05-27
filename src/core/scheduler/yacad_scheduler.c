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
#include "common/message/yacad_message_visitor.h"
#include "common/smq/yacad_zmq.h"

#define INPROC_CHECK_ADDRESS "inproc://scheduler-check"
#define INPROC_RUN_ADDRESS "inproc://scheduler-run"
#define MSG_START "start"
#define MSG_CHECK "check"
#define MSG_STOP  "stop"
#define MSG_EVENT "event"

typedef struct {
     struct timeval time; // time of the next check
     int confgen;
} next_check_t;

typedef struct yacad_scheduler_impl_s {
     yacad_scheduler_t fn;
     yacad_conf_t *conf;
     yacad_database_t *database;
     yacad_tasklist_t *tasklist;
     next_check_t worker_next_check;
     pthread_t worker;
     bool_t running;
     bool_t publish;
     yacad_zmq_socket_t *zevents;
     yacad_zmq_socket_t *zrunner;
} yacad_scheduler_impl_t;

static bool_t is_done(yacad_scheduler_impl_t *this) {
     return !this->running;
}

static void stop(yacad_scheduler_impl_t *this) {
     yacad_zmq_socket_t *zworker = yacad_zmq_socket_connect(this->conf->log, INPROC_RUN_ADDRESS, ZMQ_PAIR);
     this->running = false;
     if (zworker != NULL) {
          zworker->send(zworker, MSG_STOP);
          zworker->free(zworker);
     }
}

static void free_(yacad_scheduler_impl_t *this) {
     this->tasklist->free(this->tasklist);
     this->database->free(this->database);
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

typedef struct {
     bool_t running;
     bool_t check;
     yacad_scheduler_impl_t *this;
     yacad_zmq_socket_t *zscheduler_check;
} worker_context_t;

static bool_t worker_wait_start(yacad_zmq_poller_t *poller, yacad_zmq_socket_t *socket, const char *message, void *data) {
     worker_context_t *context = (worker_context_t*)data;

     if (data != NULL && !strcmp(data, MSG_START)) {
          context->running = true;
          return false;
     }
     return true;
}

static bool_t worker_on_timeout(yacad_zmq_poller_t *poller, void *data) {
     worker_context_t *context = (worker_context_t*)data;
     struct timeval now;

     gettimeofday(&now, NULL);
     if (timercmp(&(context->this->worker_next_check.time), &now, <)) {
          context->zscheduler_check->send(context->zscheduler_check, MSG_CHECK);
          check = true;
     }

     return true;
}

static void worker_timeout(yacad_zmq_poller_t *poller, struct timeval *timeout, void *data) {
     worker_context_t *context = (worker_context_t*)data;

     *timeout = context->this->worker_next_check.time;
}

static bool_t worker_on_pollin(yacad_zmq_poller_t *poller, yacad_zmq_socket_t *socket, const char *message, void *data) {
     worker_context_t *context = (worker_context_t*)data;

     if (message != NULL && !strcmp(message, MSG_STOP)) {
          context->running = false;
          return false;
     }
     return true;
}

static void *worker_routine(yacad_scheduler_impl_t *this) {
     worker_context_t context = {false, false, this};
     yacad_zmq_socket_t *zscheduler_run;
     yacad_zmq_poller_t *zpoller;
     int confgen;
     int i;
     char buffer[16];
     size_t n = sizeof(buffer);
     struct timeval now;
     long timeout;

     set_thread_name("schedule worker");

     zscheduler_run = yacad_zmq_socket_bind(this->conf->log, INPROC_RUN_ADDRESS, ZMQ_PAIR);
     if (zscheduler_run == NULL) {
          this->conf->log(error, "Invalid 0MQ scheduler run socket");
     } else {
          zpoller = yacad_zmq_poller_new(this->conf->log);
          if (zpoller == NULL) {
               this->conf->log(error, "Invalid 0MQ scheduler poller");
          } else {
               zpoller->on_pollin(zpoller, zscheduler_run, worker_wait_start);
               zpoller->run(zpoller, &context);

               context.zscheduler_check = yacad_zmq_socket_connect(this->conf->log, INPROC_CHECK_ADDRESS, ZMQ_PAIR);
               if(context.zscheduler_check == NULL) {
                    this->conf->log(error, "Invalid 0MQ scheduler check socket");
               } else {
                    zpoller->set_timeout(zpoller, worker_timeout, worker_on_timeout);
                    zpoller->on_pollin(zpoller, zscheduler_run, worker_on_pollin);
                    zpoller->run(zpoller);

                    context.zscheduler_check->free(context.zscheduler_check);
               }
               zpoller->free(zpoller);
          }
          zscheduler_run->free(zscheduler_run);
     }

     return this;
}

static void iterate_check_project(cad_hash_t *projects, int index, const char *project_name, yacad_project_t *project, yacad_scheduler_impl_t *this) {
     yacad_task_t *task = project->check(project);
     if (task != NULL) {
          this->tasklist->add(this->tasklist, task);
          this->publish = true;
     }
}

static void reply_get_task(yacad_scheduler_impl_t *this, yacad_runnerid_t *runnerid, yacad_task_t *task) {
     yacad_message_reply_get_task_t *message;
     yacad_scm_t *scm = NULL;
     cad_hash_t *projects;
     yacad_project_t *project;
     char *serial = NULL;

     if (task != NULL) {
          projects = this->conf->get_projects(this->conf);
          project = projects->get(projects, task->get_project_name(task));
          scm = project->get_scm(project);
     }
     message = yacad_message_reply_get_task_new(this->conf->log, runnerid, scm, task);

     serial = I(message)->serialize(I(message));

     this->zrunner->send(this->zrunner, serial);

     free(serial);
     I(message)->free(I(message));
}

static void reply_set_result(yacad_scheduler_impl_t *this, yacad_runnerid_t *runnerid) {
     yacad_message_reply_set_result_t *message;
     char *serial = NULL;

     message = yacad_message_reply_set_result_new(this->conf->log, runnerid);

     serial = I(message)->serialize(I(message));

     this->zrunner->send(this->zrunner, serial);

     free(serial);
     I(message)->free(I(message));
}

typedef struct {
     yacad_message_visitor_t fn;
     yacad_scheduler_impl_t *scheduler;
} yacad_scheduler_message_visitor_t;

static void visit_query_get_task(yacad_scheduler_message_visitor_t *this, yacad_message_query_get_task_t *message) {
     yacad_runnerid_t *runnerid = message->get_runnerid(message);
     yacad_task_t *task;

     if (runnerid == NULL) {
          this->scheduler->conf->log(warn, "Missing runnerid");
     } else {
          task = this->scheduler->tasklist->get(this->scheduler->tasklist, runnerid);
          if (task == NULL) {
               this->scheduler->conf->log(info, "No suitable task for runnerid: %s", runnerid->serialize(runnerid));
               reply_get_task(this->scheduler, runnerid, NULL);
          } else {
               this->scheduler->conf->log(info, "Sending task %lu to runnerid: %s", task->get_id(task), runnerid->serialize(runnerid));
               reply_get_task(this->scheduler, runnerid, task);
               task->set_status(task, task_running);
          }
     }
}

static void visit_reply_get_task(yacad_scheduler_message_visitor_t *this, yacad_message_reply_get_task_t *message) {
     this->scheduler->conf->log(warn, "Unexpected message");
}

static void visit_query_set_result(yacad_scheduler_message_visitor_t *this, yacad_message_query_set_result_t *message) {
     yacad_runnerid_t *runnerid = message->get_runnerid(message);
     yacad_task_t *task = message->get_task(message), *next_task;
     cad_hash_t *projects;
     yacad_project_t *project;

     if (runnerid == NULL) {
          this->scheduler->conf->log(warn, "Missing runnerid");
     } else if (task == NULL) {
          this->scheduler->conf->log(warn, "Missing task");
     } else {
          projects = this->scheduler->conf->get_projects(this->scheduler->conf);
          project = projects->get(projects, task->get_project_name(task));
          if (project == NULL) {
               this->scheduler->conf->log(warn, "Unknown project: %s", task->get_project_name(task));
          } else if (!message->is_successful(message)) {
               this->scheduler->tasklist->set_task_aborted(this->scheduler->tasklist, task);
               reply_set_result(this->scheduler, runnerid);
          } else {
               this->scheduler->tasklist->set_task_done(this->scheduler->tasklist, task);
               next_task = project->next_task(project, task);
               if (next_task != NULL) {
                    this->scheduler->tasklist->add(this->scheduler->tasklist, next_task);
                    this->scheduler->publish = true;
               }
               reply_set_result(this->scheduler, runnerid);
          }
     }
}

static void visit_reply_set_result(yacad_scheduler_message_visitor_t *this, yacad_message_reply_set_result_t *message) {
     this->scheduler->conf->log(warn, "Unexpected message");
}

static yacad_message_visitor_t scheduler_message_visitor_fn = {
     .visit_query_get_task = (yacad_message_visitor_visit_query_get_task_fn)visit_query_get_task,
     .visit_reply_get_task = (yacad_message_visitor_visit_reply_get_task_fn)visit_reply_get_task,
     .visit_query_set_result = (yacad_message_visitor_visit_query_set_result_fn)visit_query_set_result,
     .visit_reply_set_result = (yacad_message_visitor_visit_reply_set_result_fn)visit_reply_set_result,
};

static bool_t on_pollin_zworker_check(yacad_zmq_poller_t *poller, yacad_zmq_socket_t *socket, const char *strmsg, void *data) {
     yacad_scheduler_impl_t *this = (yacad_scheduler_impl_t*)data;
     cad_hash_t *projects;

     if (strmsg != NULL) {
          if (!strcmp(strmsg, MSG_STOP)) {
               this->running = false;
          } else if (!strcmp(strmsg, MSG_CHECK)) {
               this->publish = false;

               this->conf->log(debug, "Checking projects");
               projects = this->conf->get_projects(this->conf);
               projects->iterate(projects, (cad_hash_iterator_fn)iterate_check_project, this);

               if (this->publish) {
                    this->conf->log(debug, "Publishing event to %s", this->conf->get_events_name(this->conf));
                    this->zevents->send(this->zevents, MSG_EVENT);
               }
          }
     }

     return this->running;
}

static bool_t on_pollin_zrunner(yacad_zmq_poller_t *poller, yacad_zmq_socket_t *socket, const char *strmsg, void *data) {
     yacad_scheduler_impl_t *this = (yacad_scheduler_impl_t*)data;

     yacad_scheduler_message_visitor_t v = { scheduler_message_visitor_fn, this };
     yacad_message_t *message;

     message = yacad_message_unserialize(this->conf->log, strmsg, NULL);
     if (message == NULL) {
          this->conf->log(warn, "Received invalid message: %s", strmsg);
     } else {
          this->publish = false;

          message->accept(message, I(&v));
          message->free(message);

          if (this->publish) {
               this->conf->log(debug, "Publishing event to %s", this->conf->get_events_name(this->conf));
               this->zevents->send(this->zevents, MSG_EVENT);
          }
     }

     return this->running;
}

static void run(yacad_scheduler_impl_t *this) {
     yacad_zmq_socket_t *zworker_check;
     yacad_zmq_socket_t *zworker_run;
     yacad_zmq_poller_t *zpoller;

     zworker_check = yacad_zmq_socket_bind(this->conf->log, INPROC_CHECK_ADDRESS, ZMQ_PAIR);
     if (zworker_check == NULL) {
          this->conf->log(error, "Could not connect zworker_check\n");
     } else {
          this->zrunner = yacad_zmq_socket_bind(this->conf->log, this->conf->get_endpoint_name(this->conf), ZMQ_REP);
          if (this->zrunner == NULL) {
               this->conf->log(error, "Could not connect zrunner to %s\n", this->conf->get_endpoint_name(this->conf));
          } else {
               this->zevents = yacad_zmq_socket_bind(this->conf->log, this->conf->get_events_name(this->conf), ZMQ_PUB);
               if (this->zevents == NULL) {
                    this->conf->log(error, "Could not connect zevents to %s\n", this->conf->get_endpoint_name(this->conf));
               } else {
                    zworker_run = yacad_zmq_socket_connect(this->conf->log, INPROC_RUN_ADDRESS, ZMQ_PAIR);
                    if (zworker_run == NULL) {
                         this->conf->log(error, "Could not connect zworker_run\n");
                    } else {
                         zworker_run->send(zworker_run, MSG_START);
                         zworker_run->free(zworker_run);
                         this->running = true;

                         zpoller = yacad_zmq_poller_new(this->conf->log);
                         zpoller->on_pollin(zpoller, zworker_check, on_pollin_zworker_check);
                         zpoller->on_pollin(zpoller, this->zrunner, on_pollin_zrunner);

                         zpoller->run(zpoller, this);

                         zpoller->free(zpoller);
                         free(strmsg);
                    }
                    this->zevents->free(this->zevents);
               }
               this->zrunner->free(this->zrunner);
          }
          zworker_check->free(zworker_check);
     }
}

static yacad_scheduler_t impl_fn = {
     .is_done = (yacad_scheduler_is_done_fn)is_done,
     .run = (yacad_scheduler_run_fn)run,
     .stop = (yacad_scheduler_stop_fn)stop,
     .free = (yacad_scheduler_free_fn)free_,
};

yacad_scheduler_t *yacad_scheduler_new(yacad_conf_t *conf) {
     yacad_scheduler_impl_t *result = malloc(sizeof(yacad_scheduler_impl_t));
     yacad_database_t *database = yacad_database_new(conf->log, conf->get_database_name(conf));
     struct timeval now;
     gettimeofday(&now, NULL);
     result->fn = impl_fn;
     result->conf = conf;
     result->running = false;
     result->worker_next_check.confgen = -1;
     result->worker_next_check.time = now;
     result->database = database;
     result->tasklist = yacad_tasklist_new(conf->log, database);
     pthread_create(&(result->worker), NULL, (void*(*)(void*))worker_routine, result);
     return I(result);
}
