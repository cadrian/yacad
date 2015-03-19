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

#ifndef __YACAD_MESSAGE_QUERY_GET_TASK_H__
#define __YACAD_MESSAGE_QUERY_GET_TASK_H__

#include "yacad_message.h"
#include "common/runnerid/yacad_runnerid.h"

typedef yacad_runnerid_t *(*yacad_message_query_get_task_get_runnerid_fn)(yacad_message_reply_get_task_t *this);

struct yacad_message_query_get_task_s {
     yacad_message_t fn;
     yacad_message_query_get_task_get_runnerid_fn get_runnerid;
};

yacad_message_query_get_task_t *yacad_message_query_get_task_new(logger_t log, yacad_runnerid_t *runnerid);
yacad_message_query_get_task_t *yacad_message_query_get_task_unserialize(logger_t log, json_value_t *jserial, cad_hash_t *env);

#endif /* __YACAD_MESSAGE_QUERY_GET_TASK_H__ */
