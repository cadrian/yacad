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

#include "conf/yacad_conf.h"
#include "scheduler/yacad_scheduler.h"

static yacad_conf_t *conf = NULL;
static yacad_scheduler_t *scheduler = NULL;

static void handle_signal(int sig) {
     if (conf != NULL) {
          conf->log(info, "Received signal %d: %s", sig, strsignal(sig));
     }
     if (scheduler != NULL) {
          scheduler->stop(scheduler);
     }
}

int main(int argc, const char * const *argv) {
     struct sigaction action;

     set_thread_name("core");

     get_zmq_context();

     conf = yacad_conf_new();
     conf->log(info, "yaCAD core version %s - READY", yacad_version());

     action.sa_handler = handle_signal;
     action.sa_flags = 0;
     sigemptyset(&action.sa_mask);
     if (sigaction(SIGINT, &action, NULL) < 0) {
          conf->log(warn, "Could not set SIGINT action handler");
     }
     if (sigaction(SIGTERM, &action, NULL) < 0) {
          conf->log(warn, "Could not set SIGTERM action handler");
     }

     scheduler = yacad_scheduler_new(conf);
     scheduler->run(scheduler);

     conf->log(info, "yaCAD stopped.");

     del_zmq_context();

     scheduler->free(scheduler);
     //conf->free(conf); // TODO to check

     return 0;
}
