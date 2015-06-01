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

#include "test.h"

#include "common/runnerid/yacad_runnerid.h"

int test(void) {
     int result = 0;
     logger_t log = get_logger(trace);

     yacad_runnerid_t *ser, *unser;

     unser = yacad_runnerid_unserialize(log, "{}");
     assert(unser != NULL);
     assert(unser->get_name(unser) == NULL);
     assert(unser->get_arch(unser) == NULL);
     ser = yacad_runnerid_unserialize(log, unser->serialize(unser));
     assert(unser->same_as(unser, ser));
     assert(ser->same_as(ser, unser));
     ser->free(ser);
     unser->free(unser);

     //ser = yacad_runnerid_unserialize(log, "{\"arch\":\"i386\"}");
     //ser = yacad_runnerid_unserialize(log, "{\"name\":\"foo\"}");
     //ser = yacad_runnerid_unserialize(log, "{\"name\":\"foo\",\"arch\":\"amd64\"}");
     //new = yacad_runnerid_new(log, desc);

     return result;
}
