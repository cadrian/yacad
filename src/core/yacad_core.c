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
     yacad_conf_t *conf;
     yacad_scheduler_t *scheduler;

     get_zmq_context();

     conf = yacad_conf_new();
     conf->log(info, "yaCAD core version %s - READY", yacad_version());

     scheduler = yacad_scheduler_new(conf);
     scheduler->run(scheduler);

     scheduler->free(scheduler);
     conf->free(conf);

     del_zmq_context();

     return 0;
}
