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

#include "scheduler/yacad_scheduler.h"

int main(int argc, const char * const *argv) {
     yacad_scheduler_t *scheduler = yacad_scheduler_new();
     while (!scheduler->is_done(scheduler)) {
          scheduler->wait_and_run(scheduler);
     }
     scheduler->free(scheduler);
     return 0;
}
