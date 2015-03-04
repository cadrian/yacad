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

#include <stdlib.h>

#include "yacad_conf.h"

typedef struct yacad_conf_impl_s {
     yacad_conf_t fn;
     const char *database_name;
} yacad_conf_impl_t;

static const char *get_database_name(yacad_conf_impl_t *this) {
     return this->database_name;
}

static void free_(yacad_conf_impl_t *this) {
     free(this);
}

static yacad_conf_t impl_fn =  {
     .get_database_name = (yacad_conf_get_database_name_fn)get_database_name,
     .free = (yacad_conf_free_fn)free_,
};

yacad_conf_t *yacad_conf_new(void) {
     yacad_conf_impl_t *result = malloc(sizeof(yacad_conf_impl_t));
     result->fn = impl_fn;
     result->database_name = "yacad.db";
     return &(result->fn);
}
