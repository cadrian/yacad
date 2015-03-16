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

#include "conf/yacad_conf.h"
#include "scheduler/yacad_scheduler.h"
#include "scheduler/yacad_events.h"

int main(int argc, const char * const *argv) {
     yacad_conf_t *conf = yacad_conf_new();
     yacad_events_t *events = yacad_events_new();
     yacad_scheduler_t *scheduler = yacad_scheduler_new(conf);
     bool_t done = false;

     conf->log(info, "yaCAD core version %s - READY\n", yacad_version());

     do {
          scheduler->prepare(scheduler, events);
          done = !events->step(events);
     } while (!done);

     scheduler->free(scheduler);
     events->free(events);
     conf->free(conf);
     return 0;
}
