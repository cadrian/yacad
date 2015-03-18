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

typedef struct yacad_runnerid_impl_s {
     yacad_runnerid_t fn;
     char *name; // -> _
     char *arch; // -> _ + name
     size_t len;
     char _[0];
} yacad_runnerid_impl_t;

static const char *get_name(yacad_runnerid_impl_t *this) {
     return this->name;
}

static const char *get_arch(yacad_runnerid_impl_t *this) {
     return this->arch;
}

static size_t serialize_to(yacad_runnerid_impl_t *this, char *out, size_t size) {
     size_t result;
     if (this->name == NULL) {
          if (this->arch == NULL) {
               result = snprintf(out, size, "{}");
          } else {
               result = snprintf(out, size, "{\"arch\":\"%s\"}", this->arch);
          }
     } else if (this->arch == NULL) {
          result = snprintf(out, size, "{\"name\":\"%s\"}", this->name);
     } else {
          result = snprintf(out, size, "{\"name\":\"%s\",\"arch\":\"%s\"}", this->name, this->arch);
     }
     return result;
}

static const char *serialize(yacad_runnerid_impl_t *this) {
     static size_t capacity = 0;
     static char *result = NULL;
     size_t n;

     n = serialize_to(this, result, capacity);
     if (n > capacity) {
          if (capacity == 0) {
               capacity = 4096;
          }
          while (capacity < n) {
               capacity *= 2;
          }
          result = realloc(result, capacity);
          serialize_to(this, result, capacity);
     }

     return result;
}

static bool_t same_as(yacad_runnerid_impl_t *this, yacad_runnerid_impl_t *other) {
     bool_t result = this->len == other->len;
     if (result) {
          result = !memcmp(this->_, other->_, this->len);
     }
     return result;
}

static int match(yacad_runnerid_impl_t *this, yacad_runnerid_impl_t *other) {
     int result = -1;
     if (this->name != NULL && other->name != NULL) {
          if (!strcmp(this->name, other->name)) {
               result += 5;
          }
     }
     if (this->arch != NULL && other->arch != NULL) {
          if (!strcmp(this->arch, other->arch)) {
               result += 2;
          }
     }
     return result;
}

static void free_(yacad_runnerid_impl_t *this) {
     free(this);
}

static yacad_runnerid_t impl_fn = {
     .get_name = (yacad_runnerid_get_name_fn)get_name,
     .get_arch = (yacad_runnerid_get_arch_fn)get_arch,
     .serialize = (yacad_runnerid_serialize_fn)serialize,
     .same_as = (yacad_runnerid_same_as_fn)same_as,
     .match = (yacad_runnerid_match_fn)match,
     .free = (yacad_runnerid_free_fn)free_,
};

yacad_runnerid_t *yacad_runnerid_unserialize(logger_t log, char *serial) {
     yacad_runnerid_t *result = NULL;
     json_input_stream_t *in = new_json_input_stream_from_string(serial, stdlib_memory);
     json_value_t *desc = json_parse(in, NULL, stdlib_memory);
     in->free(in);
     if (desc != NULL) {
          result = yacad_runnerid_new(log, desc);
          desc->free(desc);
     }
     return result;
}

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
     result->len = szname + szarch;
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
