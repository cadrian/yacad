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
     bool_t running;
} yacad_scheduler_impl_t;

static bool_t is_done(yacad_scheduler_impl_t *this) {
     return !this->running;
}

static void stop(yacad_scheduler_impl_t *this) {
     void *zcontext = get_zmq_context();
     void *zworker = zmq_socket(zcontext, ZMQ_PAIR);
     if (zworker != NULL) {
          if (zmqcheck(this->conf->log, zmq_connect(zworker, INPROC_RUN_ADDRESS), error)) {
               zmqcheck(this->conf->log, zmq_send(zworker, MSG_STOP, strlen(MSG_STOP), 0), error);
               zmqcheck(this->conf->log, zmq_disconnect(zworker, INPROC_RUN_ADDRESS), error);
          }
          zmqcheck(this->conf->log, zmq_close(zworker), error);
     }
     this->running = false;
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
     }
}

static void send_task(yacad_scheduler_impl_t *this, yacad_task_t *task, void *zrunner) {
     char *src, *run, *serial;
     json_output_stream_t *out;
     json_visitor_t *writer;
     size_t n;
     json_value_t *jsource = task->get_source(task);
     json_value_t *jrun = task->get_run(task);

     out = new_json_output_stream_from_string(&src, stdlib_memory);
     writer = json_write_to(out, stdlib_memory, 0);
     jsource->accept(jsource, writer);
     writer->free(writer);
     out->free(out);

     out = new_json_output_stream_from_string(&run, stdlib_memory);
     writer = json_write_to(out, stdlib_memory, 0);
     jrun->accept(jrun, writer);
     writer->free(writer);
     out->free(out);

     n = snprintf("", 0, "{\"id\":%lu,\"source\":%s,\"run\":%s}", task->get_id(task), src, run) + 1;
     serial = malloc(n);
     snprintf(serial, n, "{\"id\":%lu,\"source\":%s,\"run\":%s}", task->get_id(task), src, run);

     zmqcheck(this->conf->log, zmq_send(zrunner, serial, n, 0), warn);

     free(serial);
     free(src);
     free(run);
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
               zmqcheck(this->scheduler->conf->log, zmq_send(this->zrunner, "", 0, 0), warn);
          } else {
               this->scheduler->conf->log(info, "Sending task %lu to runnerid: %s", task->get_id(task), runnerid->serialize(runnerid));
               send_task(this->scheduler, task, this->zrunner);
          }
     }
}

static void visit_reply_get_task(yacad_scheduler_message_visitor_t *this, yacad_message_reply_get_task_t *message) {
     this->scheduler->conf->log(warn, "Unexpected message");
}

static void visit_query_set_result(yacad_scheduler_message_visitor_t *this, yacad_message_query_set_result_t *message) {
     this->scheduler->conf->log(warn, "Not yet implemented"); // TODO
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
     int i;
     char buffer[512]; // TODO replace by a 0MQ message to ensure big sizes
     size_t n = sizeof(buffer);
     cad_hash_t *projects;
     zmq_pollitem_t zitems[] = {
          {zworker_check, 0, ZMQ_POLLIN, 0},
          {zrunner, 0, ZMQ_POLLIN, 0},
     };

     yacad_scheduler_message_visitor_t v = { scheduler_message_visitor_fn, this, zrunner };
     yacad_message_t *message;

     if (zworker_check != NULL && zworker_run != NULL && zrunner != NULL) {
          if (zmqcheck(this->conf->log, zmq_bind(zworker_check, INPROC_CHECK_ADDRESS), error)
              && zmqcheck(this->conf->log, zmq_bind(zrunner, this->conf->get_endpoint_name(this->conf)), error)) {

               if (zmqcheck(this->conf->log, zmq_connect(zworker_run, INPROC_RUN_ADDRESS), error)) {
                    if (zmqcheck(this->conf->log, zmq_send(zworker_run, MSG_START, strlen(MSG_START), 0), error)) {
                         this->running = true;
                    }
                    zmqcheck(this->conf->log, zmq_disconnect(zworker_run, INPROC_RUN_ADDRESS), warn);
               }

               while (this->running) {
                    if (!zmqcheck(this->conf->log, zmq_poll(zitems, 2, -1), debug)) {
                         this->running = false;
                    } else if (zitems[0].revents & ZMQ_POLLIN) {
                         if (!zmqcheck(this->conf->log, i = zmq_recv(zworker_check, buffer, n, 0), error)) {
                              this->running = false;
                         } else if (!strncmp(buffer, MSG_STOP, i)) {
                              this->running = false;
                         } else if (!strncmp(buffer, MSG_CHECK, i)) {
                              this->conf->log(debug, "Checking projects");
                              projects = this->conf->get_projects(this->conf);
                              projects->iterate(projects, (cad_hash_iterator_fn)iterate_check_project, this);
                         }
                    } else if (zitems[1].revents & ZMQ_POLLIN) {
                         if (!zmqcheck(this->conf->log, i = zmq_recv(zrunner, buffer, n, 0), error)) {
                              this->running = false;
                         } else {
                              buffer[i] = '\0';
                              message = yacad_message_unserialize(this->conf->log, buffer, NULL);
                              if (message == NULL) {
                                   this->conf->log(warn, "Received invalid message: %s", buffer);
                              } else {
                                   message->accept(message, I(&v));
                                   message->free(message);
                              }
                         }
                    }
               }
          }

          zmqcheck(this->conf->log, zmq_close(zworker_run), warn);
          zmqcheck(this->conf->log, zmq_close(zworker_check), warn);
          zmqcheck(this->conf->log, zmq_close(zrunner), warn);
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
     struct timeval now;
     gettimeofday(&now, NULL);
     result->fn = impl_fn;
     result->conf = conf;
     result->running = false;
     result->worker_next_check.confgen = -1;
     result->worker_next_check.time = now;
     result->tasklist = yacad_tasklist_new(conf->log, conf->get_database_name(conf));
     pthread_create(&(result->worker), NULL, (void*(*)(void*))worker_routine, result);
     return I(result);
}
