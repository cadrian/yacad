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
} yacad_scheduler_impl_t;

static bool_t is_done(yacad_scheduler_impl_t *this) {
     return !this->running;
}

static void stop(yacad_scheduler_impl_t *this) {
     void *zcontext = get_zmq_context();
     void *zworker = zmq_socket(zcontext, ZMQ_PAIR);
     this->running = false;
     if (zworker != NULL) {
          if (zmqcheck(this->conf->log, zmq_connect(zworker, INPROC_RUN_ADDRESS), error)) {
               zmqcheck(this->conf->log, zmq_send(zworker, MSG_STOP, strlen(MSG_STOP), 0), error);
               zmqcheck(this->conf->log, zmq_disconnect(zworker, INPROC_RUN_ADDRESS), error);
          }
          zmqcheck(this->conf->log, zmq_close(zworker), error);
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

static void *worker_routine(yacad_scheduler_impl_t *this) {
     void *zcontext = get_zmq_context();
     void *zscheduler_run = zmq_socket(zcontext, ZMQ_PAIR);
     void *zscheduler_check = zmq_socket(zcontext, ZMQ_PAIR);
     int confgen;
     bool_t running = false;
     bool_t check = false;
     int i;
     char buffer[16];
     size_t n = sizeof(buffer);
     struct timeval now;
     zmq_pollitem_t zitems[] = {
          {zscheduler_run, 0, ZMQ_POLLIN, 0},
     };
     long timeout;

     set_thread_name("schedule worker");

     if (zscheduler_run == NULL || zscheduler_check == NULL) {
          this->conf->log(error, "Invalid 0MQ scheduler sockets");
          this->conf->log(debug, "zscheduler_run=%p zscheduler_check=%p", zscheduler_run, zscheduler_check);
     } else {
          if (zmqcheck(this->conf->log, zmq_bind(zscheduler_run, INPROC_RUN_ADDRESS), error)) {
               do {
                    if (!zmqcheck(this->conf->log, i = zmq_recv(zscheduler_run, buffer, n, 0), error)) {
                         this->conf->log(debug, "Not running scheduler worker!!!");
                         break; // running stays false
                    } else if (!strncmp(buffer, MSG_START, i)) {
                         running = true;
                    }
               } while (!running);

               if (zmqcheck(this->conf->log, zmq_connect(zscheduler_check, INPROC_CHECK_ADDRESS), error)) {
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

                         if (!zmqcheck(this->conf->log, zmq_poll(zitems, 1, timeout * 1000L), debug)) {
                              running = false;
                         } else if (zitems[0].revents & ZMQ_POLLIN) {
                              if (!zmqcheck(this->conf->log, i = zmq_recv(zscheduler_run, buffer, n, 0), error)) {
                                   running = false;
                              } else if (!strncmp(buffer, MSG_STOP, i)) {
                                   running = false;
                              }
                         } else {
                              gettimeofday(&now, NULL);
                              if (timercmp(&(this->worker_next_check.time), &now, <)) {
                                   if (zmqcheck(this->conf->log, zmq_send(zscheduler_check, MSG_CHECK, strlen(MSG_CHECK), 0), warn)) {
                                        check = true;
                                   }
                              }
                         }
                    }
               }
          }
     }

     zmqcheck(this->conf->log, zmq_close(zscheduler_check), warn);
     zmqcheck(this->conf->log, zmq_close(zscheduler_run), warn);

     return this;
}

static void iterate_check_project(cad_hash_t *projects, int index, const char *project_name, yacad_project_t *project, yacad_scheduler_impl_t *this) {
     yacad_task_t *task = project->check(project);
     if (task != NULL) {
          this->tasklist->add(this->tasklist, task);
          this->publish = true;
     }
}

static void reply_get_task(yacad_scheduler_impl_t *this, yacad_runnerid_t *runnerid, yacad_task_t *task, void *zrunner) {
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

     zmqcheck(this->conf->log, zmq_send(zrunner, serial, strlen(serial) + 1, 0), warn);

     free(serial);
     I(message)->free(I(message));
}

static void reply_set_result(yacad_scheduler_impl_t *this, yacad_runnerid_t *runnerid, void *zrunner) {
     yacad_message_reply_set_result_t *message;
     char *serial = NULL;

     message = yacad_message_reply_set_result_new(this->conf->log, runnerid);

     serial = I(message)->serialize(I(message));

     zmqcheck(this->conf->log, zmq_send(zrunner, serial, strlen(serial) + 1, 0), warn);

     free(serial);
     I(message)->free(I(message));
}

typedef struct {
     yacad_message_visitor_t fn;
     yacad_scheduler_impl_t *scheduler;
     void *zrunner;
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
               reply_get_task(this->scheduler, runnerid, NULL, this->zrunner);
          } else {
               this->scheduler->conf->log(info, "Sending task %lu to runnerid: %s", task->get_id(task), runnerid->serialize(runnerid));
               reply_get_task(this->scheduler, runnerid, task, this->zrunner);
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
               reply_set_result(this->scheduler, runnerid, this->zrunner);
          } else {
               this->scheduler->tasklist->set_task_done(this->scheduler->tasklist, task);
               next_task = project->next_task(project, task);
               if (next_task != NULL) {
                    this->scheduler->tasklist->add(this->scheduler->tasklist, next_task);
                    this->scheduler->publish = true;
               }
               reply_set_result(this->scheduler, runnerid, this->zrunner);
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

static void run(yacad_scheduler_impl_t *this) {
     void *zcontext = get_zmq_context();
     void *zworker_check = zmq_socket(zcontext, ZMQ_PAIR);
     void *zworker_run = zmq_socket(zcontext, ZMQ_PAIR);
     void *zrunner = zmq_socket(zcontext, ZMQ_REP);
     void *zevents = zmq_socket(zcontext, ZMQ_PUB);
     int n, c = 0;
     zmq_msg_t msg;
     char *strmsg = NULL;
     cad_hash_t *projects;
     zmq_pollitem_t zitems[] = {
          {zworker_check, 0, ZMQ_POLLIN, 0},
          {zrunner, 0, ZMQ_POLLIN, 0},
     };

     yacad_scheduler_message_visitor_t v = { scheduler_message_visitor_fn, this, zrunner };
     yacad_message_t *message;

     if (zworker_check != NULL && zworker_run != NULL && zrunner != NULL) {
          if (zmqcheck(this->conf->log, zmq_bind(zworker_check, INPROC_CHECK_ADDRESS), error)
              && zmqcheck(this->conf->log, zmq_bind(zrunner, this->conf->get_endpoint_name(this->conf)), error)
              && zmqcheck(this->conf->log, zmq_bind(zevents, this->conf->get_events_name(this->conf)), error)) {

               if (zmqcheck(this->conf->log, zmq_connect(zworker_run, INPROC_RUN_ADDRESS), error)) {
                    if (zmqcheck(this->conf->log, zmq_send(zworker_run, MSG_START, strlen(MSG_START), 0), error)) {
                         this->running = true;
                    }
                    zmqcheck(this->conf->log, zmq_disconnect(zworker_run, INPROC_RUN_ADDRESS), warn);
               }

               while (this->running) {
                    this->publish = false;
                    if (!zmqcheck(this->conf->log, zmq_poll(zitems, 2, -1), debug)) {
                         this->running = false;
                    } else if (zitems[0].revents & ZMQ_POLLIN) {
                         if (!zmqcheck(this->conf->log, zmq_msg_init(&msg), error) ||
                             !zmqcheck(this->conf->log, n = zmq_msg_recv(&msg, zworker_check, 0), error)) {
                              this->running = false;
                         } else {
                              if (n > 0) {
                                   if (c < n) {
                                        if (c == 0) {
                                             strmsg = malloc(c = 4096);
                                        } else {
                                             do {
                                                  c *= 2;
                                             } while (c < n);
                                             strmsg = realloc(strmsg, c);
                                        }
                                   }
                                   memcpy(strmsg, zmq_msg_data(&msg), n);
                                   zmqcheck(this->conf->log, zmq_msg_close(&msg), warn);
                                   strmsg[n] = '\0';
                                   if (!strcmp(strmsg, MSG_STOP)) {
                                        this->running = false;
                                   } else if (!strcmp(strmsg, MSG_CHECK)) {
                                        this->conf->log(debug, "Checking projects");
                                        projects = this->conf->get_projects(this->conf);
                                        projects->iterate(projects, (cad_hash_iterator_fn)iterate_check_project, this);
                                   }
                              } else {
                                   zmqcheck(this->conf->log, zmq_msg_close(&msg), warn);
                              }
                         }
                    } else if (zitems[1].revents & ZMQ_POLLIN) {
                         if (!zmqcheck(this->conf->log, zmq_msg_init(&msg), error) ||
                             !zmqcheck(this->conf->log, n = zmq_msg_recv(&msg, zrunner, 0), error)) {
                              this->running = false;
                         } else {
                              if (n > 0) {
                                   if (c < n) {
                                        if (c == 0) {
                                             strmsg = malloc(c = 4096);
                                        } else {
                                             do {
                                                  c *= 2;
                                             } while (c < n);
                                             strmsg = realloc(strmsg, c);
                                        }
                                   }
                                   memcpy(strmsg, zmq_msg_data(&msg), n);
                                   zmqcheck(this->conf->log, zmq_msg_close(&msg), warn);
                                   strmsg[n] = '\0';
                                   message = yacad_message_unserialize(this->conf->log, strmsg, NULL);
                                   if (message == NULL) {
                                        this->conf->log(warn, "Received invalid message: %s", strmsg);
                                   } else {
                                        message->accept(message, I(&v));
                                        message->free(message);
                                   }
                              } else {
                                   zmqcheck(this->conf->log, zmq_msg_close(&msg), warn);
                              }
                         }
                    }

                    if (this->publish) {
                         this->conf->log(debug, "Publishing event to %s", this->conf->get_events_name(this->conf));
                         zmqcheck(this->conf->log, zmq_send(zevents, MSG_EVENT, strlen(MSG_EVENT), 0), warn);
                    }
               }
          }

          zmqcheck(this->conf->log, zmq_close(zworker_run), warn);
          zmqcheck(this->conf->log, zmq_close(zworker_check), warn);
          zmqcheck(this->conf->log, zmq_close(zrunner), warn);
          zmqcheck(this->conf->log, zmq_close(zevents), warn);
          free(strmsg);
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
