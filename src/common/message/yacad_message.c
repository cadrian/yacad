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

#include "yacad_message.h"
#include "yacad_message_query_get_task.h"
#include "yacad_message_reply_get_task.h"
#include "yacad_message_query_set_result.h"
#include "yacad_message_reply_set_result.h"
#include "common/json/yacad_json_finder.h"

yacad_message_t *yacad_message_unserialize(logger_t log, const char *serial, cad_hash_t *env) {
     yacad_message_t *result = NULL;
     json_input_stream_t *in = new_json_input_stream_from_string(serial, stdlib_memory);
     json_value_t *jserial = json_parse(in, NULL, stdlib_memory);
     yacad_json_finder_t *finder = yacad_json_finder_new(log, json_type_string, "%s");
     json_string_t *jtype;
     char *type;
     int n;

     finder->visit(finder, jserial, "type");
     jtype = finder->get_string(finder);

     n = jtype->utf8(jtype, "", 0) + 1;
     type = alloca(n);
     jtype->utf8(jtype, type, n);

     if (!strcmp(type, "query_get_task")) {
          result = (yacad_message_t *)yacad_message_query_get_task_unserialize(log, jserial, env);
     } else if (!strcmp(type, "reply_get_task")) {
          result = (yacad_message_t *)yacad_message_reply_get_task_unserialize(log, jserial, env);
     } else if (!strcmp(type, "query_set_result")) {
          result = (yacad_message_t *)yacad_message_query_set_result_unserialize(log, jserial, env);
     } else if (!strcmp(type, "reply_set_result")) {
          result = (yacad_message_t *)yacad_message_reply_set_result_unserialize(log, jserial, env);
     } else {
          log(warn, "Invalid message: %s", serial);
     }

     finder->free(finder);
     jserial->free(jserial);
     in->free(in);

     return result;
}
