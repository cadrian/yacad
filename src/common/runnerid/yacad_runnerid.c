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

#include "yacad_runnerid.h"
#include "common/json/yacad_json_finder.h"

typedef struct yacad_runnerid_ompl_s {
     yacad_runnerid_t fn;
     char *name; // -> _
     char *arch; // -> _ + name
     char _[0];
} yacad_runnerid_impl_t;

static const char *get_name(yacad_runnerid_impl_t *this) {
     return this->name;
}

static const char *get_arch(yacad_runnerid_impl_t *this) {
     return this->arch;
}

static void free_(yacad_runnerid_impl_t *this) {
     free(this);
}

static yacad_runnerid_t impl_fn = {
     .get_name = (yacad_runnerid_get_name_fn)get_name,
     .get_arch = (yacad_runnerid_get_arch_fn)get_arch,
     .free = (yacad_runnerid_free_fn)free_,
};

yacad_runnerid_t *yacad_runnerid_new(logger_t log, json_value_t *desc) {
     yacad_runnerid_impl_t *result;
     yacad_json_finder_t *v = yacad_json_finder_new(log, json_type_string, "%s");
     json_string_t *jname, *jarch;
     size_t szname = 0, szarch = 0;

     v->visit(v, desc, "name");
     jname = v->get_string(v);
     if (jname != NULL) {
          szname = jname->utf8(jname, "", 0) + 1;
     }

     v->visit(v, desc, "arch");
     jarch = v->get_string(v);
     if (jarch != NULL) {
          szarch = jarch->utf8(jarch, "", 0) + 1;
     }

     result = malloc(sizeof(yacad_runnerid_impl_t) + szname + szarch);
     result->fn = impl_fn;
     if (jname == NULL) {
          result->name = NULL;
     } else {
          result->name = result->_;
          jname->utf8(jname, result->name, szname);
     }
     if (jarch == NULL) {
          result->arch = NULL;
     } else {
          result->arch = result->_ + szname;
          jarch->utf8(jarch, result->arch, szarch);
     }

     I(v)->free(I(v));

     return I(result);
}
