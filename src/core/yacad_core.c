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

int main(int argc, const char * const *argv) {
     yacad_conf_t *conf = yacad_conf_new();
     yacad_scheduler_t *scheduler;

     conf->log(info, "yaCAD core version %s - READY\n", yacad_version());
     get_zmq_context(conf->log);

     scheduler = yacad_scheduler_new(conf);
     scheduler->run(scheduler);

     del_zmq_context(conf->log);
     scheduler->free(scheduler);
     conf->free(conf);

     return 0;
}
