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

#include "yacad_scm.h"
#include "yacad_scm_git.h"
#include "json/yacad_json_visitor.h"

yacad_scm_t *yacad_scm_new(logger_t log, json_object_t *desc, const char *root_path) {
     yacad_scm_t *result = NULL;
     yacad_json_visitor_t *t = yacad_json_visitor_new(log, json_type_string, "type");
     json_string_t *jtype;
     char *type;
     size_t n;

     t->visit(t, (json_value_t *)desc);
     jtype = t->get_string(t);
     if (jtype == NULL) {
          log(warn, "scm type not set\n");
     } else {
          n = jtype->utf8(jtype, "", 0) + 1;
          type = alloca(n);
          n = jtype->utf8(jtype, type, n);
          if (!strncmp("git", type, n)) {
               result = yacad_scm_git_new(log, root_path, desc);
          } else {
               log(warn, "scm type not supported: %s\n", type);
          }
     }

     I(t)->free(I(t));

     return result;
}
