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

#include <signal.h>

#include "yacad.h"
#include "runner/conf/yacad_conf.h"

#define INPROC_RUN_ADDRESS "inproc://scheduler-run"
#define MSG_STOP  "stop"
#define MSG_EVENT "event"

static yacad_conf_t *conf = NULL;
static bool_t running = true;

static void handle_signal(int sig) {
     void *zcontext = get_zmq_context();
     void *zrunner = zmq_socket(zcontext, ZMQ_PAIR);
     if (conf != NULL) {
          conf->log(info, "Received signal %d: %s", sig, strsignal(sig));
     }
     running = false;
     if (zrunner != NULL) {
          if (zmqcheck(this->conf->log, zmq_connect(zrunner, INPROC_RUN_ADDRESS), error)) {
               zmqcheck(this->conf->log, zmq_send(zrunner, MSG_STOP, strlen(MSG_STOP), 0), error);
               zmqcheck(this->conf->log, zmq_disconnect(zrunner, INPROC_RUN_ADDRESS), error);
          }
          zmqcheck(this->conf->log, zmq_close(zrunner), error);
     }
}

static void run() {
     void *zcontext = get_zmq_context();
     void *zscheduler_run = zmq_socket(zcontext, ZMQ_PAIR);
     void *zcore_endpoint = zmq_socket(zcontext, ZMQ_REQ);
     void *zcore_events = zmq_socket(zcontext, ZMQ_SUB);
     zmq_msg_t msg;
     char *strmsg = NULL;
     int n, c = 0;
     zmq_pollitem_t zitems[] = {
          {zscheduler_run, 0, ZMQ_POLLIN, 0},
          {zcore_endpoint, 0, ZMQ_POLLIN, 0},
          {zcore_events, 0, ZMQ_POLLIN, 0},
     };

     yacad_scheduler_message_visitor_t v = { scheduler_message_visitor_fn, this, zrunner };
     yacad_message_t *message;

     if (zscheduler_run != NULL && zcore_endpoint != NULL && zcore_events != NULL) {
          if (zmqcheck(conf->log, zmq_bind(zscheduler_run, INPROC_RUN_ADDRESS), error)
              && zmqcheck(conf->log, zmq_connect(zcore_endpoint, conf->get_enpoint_name(conf)), error)
              && zmqcheck(conf->log, zmq_connect(zcore_events, conf->get_events_name(conf)), error)) {

               do {
                    if (!zmqcheck(conf->log, zmq_poll(zitems, 3, -1), debug)) {
                         running = false;
                    } else if (zitems[0].revents & ZMQ_POLLIN) {

                         if (!zmqcheck(conf->log, zmq_msg_init(&msg), error) ||
                             !zmqcheck(conf->log, n = zmq_msg_recv(&msg, zscheduler_run, 0), error)) {
                              running = false;
                         } else if (n <= 0) {
                              zmqcheck(conf->log, zmq_msg_close(&msg), warn);
                         } else {
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
                              zmqcheck(conf->log, zmq_msg_close(&msg), warn);
                              strmsg[n] = '\0';
                              if (!strcpy(strmsg, MSG_STOP)) {
                                   running = false;
                              }
                         }

                    } else if (zitems[1].revents & ZMQ_POLLIN) {

                         // TODO
                              message = yacad_message_unserialize(conf->log, strmsg, NULL);
                              if (message == NULL) {
                                   conf->log(warn, "Received invalid message: %s", strmsg);
                              } else {
                                   message->accept(message, I(&v));
                                   message->free(message);
                              }

                    } else if (zitems[2].revents & ZMQ_POLLIN) {

                    }
               } while (running);
          }
     }

     free(strmsg);
}

int main(int argc, const char * const *argv) {
     struct sigaction action;

     set_thread_name("runner");

     get_zmq_context();

     action.sa_handler = handle_signal;
     action.sa_flags = 0;
     sigemptyset(&action.sa_mask);
     if (sigaction(SIGINT, &action, NULL) < 0) {
          conf->log(error, "Could not set SIGINT action handler");
          del_zmq_context();
          exit(1);
     }
     if (sigaction(SIGTERM, &action, NULL) < 0) {
          conf->log(error, "Could not set SIGTERM action handler");
          del_zmq_context();
          exit(1);
     }

     conf = yacad_conf_new();
     conf->log(info, "yaCAD runner version %s - READY", yacad_version());

     run();

     del_zmq_context();

     return 0;
}
