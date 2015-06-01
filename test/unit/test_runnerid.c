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
     logger_t log = get_logger(info);

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

     unser = yacad_runnerid_unserialize(log, "{\"arch\":\"i386\"}");
     assert(unser != NULL);
     assert(unser->get_name(unser) == NULL);
     assert(unser->get_arch(unser) != NULL && !strcmp(unser->get_arch(unser), "i386"));
     ser = yacad_runnerid_unserialize(log, unser->serialize(unser));
     assert(unser->same_as(unser, ser));
     assert(ser->same_as(ser, unser));
     ser->free(ser);
     unser->free(unser);

     unser = yacad_runnerid_unserialize(log, "{\"name\":\"foo\"}");
     assert(unser != NULL);
     assert(unser->get_name(unser) != NULL && !strcmp(unser->get_name(unser), "foo"));
     assert(unser->get_arch(unser) == NULL);
     ser = yacad_runnerid_unserialize(log, unser->serialize(unser));
     assert(unser->same_as(unser, ser));
     assert(ser->same_as(ser, unser));
     ser->free(ser);
     unser->free(unser);

     unser = yacad_runnerid_unserialize(log, "{\"name\":\"bar\",\"arch\":\"amd64\"}");
     assert(unser != NULL);
     assert(unser->get_name(unser) != NULL && !strcmp(unser->get_name(unser), "bar"));
     assert(unser->get_arch(unser) != NULL && !strcmp(unser->get_arch(unser), "amd64"));
     ser = yacad_runnerid_unserialize(log, unser->serialize(unser));
     assert(unser->same_as(unser, ser));
     assert(ser->same_as(ser, unser));
     ser->free(ser);
     unser->free(unser);

     return result;
}
