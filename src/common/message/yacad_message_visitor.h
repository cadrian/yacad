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

#ifndef __YACAD_MESSAGE_VISITOR_H__
#define __YACAD_MESSAGE_VISITOR_H__

#include "yacad.h"
#include "yacad_message.h"
#include "yacad_message_query_get_task.h"
#include "yacad_message_reply_get_task.h"
#include "yacad_message_query_set_result.h"
#include "yacad_message_reply_set_result.h"

typedef void (*yacad_message_visitor_visit_query_get_task_fn)(yacad_message_visitor_t *this, yacad_message_query_get_task_t *message);
typedef void (*yacad_message_visitor_visit_reply_get_task_fn)(yacad_message_visitor_t *this, yacad_message_reply_get_task_t *message);
typedef void (*yacad_message_visitor_visit_query_set_result_fn)(yacad_message_visitor_t *this, yacad_message_query_set_result_t *message);
typedef void (*yacad_message_visitor_visit_reply_set_result_fn)(yacad_message_visitor_t *this, yacad_message_reply_set_result_t *message);

struct yacad_message_visitor_s {
     yacad_message_visitor_visit_query_get_task_fn visit_query_get_task;
     yacad_message_visitor_visit_reply_get_task_fn visit_reply_get_task;
     yacad_message_visitor_visit_query_set_result_fn visit_query_set_result;
     yacad_message_visitor_visit_reply_set_result_fn visit_reply_set_result;
};

#endif /* __YACAD_MESSAGE_VISITOR_H__ */
