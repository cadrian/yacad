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
#include "common/zmq/yacad_zmq.h"

#define INPROC_RUN_ADDRESS "inproc://scheduler-run"
#define MSG_STOP  "stop"
#define MSG_EVENT "event"

static yacad_conf_t *conf = NULL;
static bool_t running = true;

static void handle_signal(int sig) {
     if (conf != NULL) {
          conf->log(info, "Received signal %d: %s", sig, strsignal(sig));
     }
     running = false;
}

static void run() {
}

int main(int argc, const char * const *argv) {
     struct sigaction action;

     set_thread_name("runner");

     yacad_zmq_init();

     action.sa_handler = handle_signal;
     action.sa_flags = 0;
     sigemptyset(&action.sa_mask);
     if (sigaction(SIGINT, &action, NULL) < 0) {
          conf->log(error, "Could not set SIGINT action handler");
          yacad_zmq_term();
          exit(1);
     }
     if (sigaction(SIGTERM, &action, NULL) < 0) {
          conf->log(error, "Could not set SIGTERM action handler");
          yacad_zmq_term();
          exit(1);
     }

     conf = yacad_conf_new();
     conf->log(info, "yaCAD runner version %s - READY", yacad_version());

     run();

     yacad_zmq_term();

     return 0;
}
