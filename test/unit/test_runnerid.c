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

static int test_unserialize(logger_t log) {
     int result = 0;
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

static int test_new(logger_t log) {
     int result = 0;
     yacad_runnerid_t *new, *ser;
     json_value_t *desc;
     json_input_stream_t *stream;

     stream = new_json_input_stream_from_string("{\"name\":\"bar\",\"arch\":\"amd64\"}", stdlib_memory);
     desc = json_parse(stream, NULL, stdlib_memory);
     new = yacad_runnerid_new(log, desc);
     desc->free(desc);
     stream->free(stream);

     assert(new != NULL);
     assert(new->get_name(new) != NULL && !strcmp(new->get_name(new), "bar"));
     assert(new->get_arch(new) != NULL && !strcmp(new->get_arch(new), "amd64"));
     ser = yacad_runnerid_unserialize(log, new->serialize(new));
     assert(new->same_as(new, ser));
     assert(ser->same_as(ser, new));
     ser->free(ser);
     new->free(new);

     return result;
}

int test(void) {
     int result = 0;
     logger_t log = get_logger(info);

     result += test_unserialize(log);
     result += test_new(log);

     return result;
}
