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
#include <stdio.h>
#include <time.h>

#include "yacad.h"
#include "yacad_cron.h"

typedef struct {
     unsigned long min:60;
     unsigned long hour:24;
     unsigned long dom:31;
     unsigned long mon:12;
     unsigned long dow:7;
} cronspec_t;

typedef struct yacad_cron_impl_s {
     yacad_cron_t fn;
     cronspec_t spec;
} yacad_cron_impl_t;

#define BIT_1(field, bit) do { (field) |= 1UL << (bit); } while(0)
#define BIT_0(field, bit) do { (field) &= ~(1UL << (bit)); } while(0)
#define ISBIT(field, bit) (((field) >> (bit)) & 1UL)

static bool_t lookup(unsigned long field, int *min, int max) {
     int i;
     bool_t result = false;
     for (i = *min; !result && i < max; i++) {
          if (ISBIT(field, i)) {
               *min = i;
               result = true;
          }
     }
     if (!result) {
          *min = *min + 1;
     }
     return result;
}

static struct timeval next(yacad_cron_impl_t *this) {
     struct timeval result;
     time_t t;
     struct tm tm;
     bool_t done = false;

     time(&t);
     localtime_r(&t, &tm);
     tm.tm_sec = 0;
     tm.tm_yday = 0;
     tm.tm_min++;

     do {
          /* inspired by http://stackoverflow.com/questions/321494/calculate-when-a-cron-job-will-be-executed-then-next-time */
          if (!lookup(this->spec.mon, &(tm.tm_mon), 12)) {
               tm.tm_mday = tm.tm_wday = tm.tm_hour = tm.tm_min = 0;
          } else if (!lookup(this->spec.dom, &(tm.tm_mday), 31)) {
               tm.tm_hour = tm.tm_min = 0;
          } else if (!lookup(this->spec.dow, &(tm.tm_wday), 7)) {
               tm.tm_hour = tm.tm_min = 0;
          } else if (!lookup(this->spec.hour, &(tm.tm_hour), 24)) {
               tm.tm_min = 0;
          } else if (!lookup(this->spec.min, &(tm.tm_min), 60)) {
          } else {
               done = true;
          }
     } while (!done);

     result.tv_sec = mktime(&tm);
     result.tv_usec = 0;
     return result;
}

static void free_(yacad_cron_impl_t *this) {
     free(this);
}

static int parse_num(const char *field, int *offset) {
     int result = 0;
     bool_t cont = true;
     do {
          switch (field[*offset]) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
               result = result * 10 + (field[*offset] - '0');
               *offset = *offset + 1;
               break;
          default:
               cont = false;
          }
     } while (cont);
     return result;
}

static unsigned long parse_field_step(unsigned long val, const char *field, int *offset, int start, int max) {
     unsigned long result = val;
     int num, i;
     if (field[*offset] == '/') {
          *offset = *offset + 1;
          num = parse_num(field, offset);
          for (i = 0; i < max - start; i++) {
               if (i % num != 0) {
                    BIT_0(result, i + start);
               }
          }
     }
     return result;
}

static unsigned long parse_field_list(const char *field, int *offset, int max) {
     unsigned long result = 0;
     unsigned long val;
     int numin, numax, i;
     bool_t cont = true;
     do {
          numin = parse_num(field, offset);

          if (field[*offset] == '-') {
               *offset = *offset + 1;
               numax = parse_num(field, offset);
          } else {
               numax = numin;
          }

          val = 0UL;
          for (i = numin; i <= numax && i < max; i++) {
               BIT_1(val, i);
          }

          if (field[*offset] == '/') {
               result |= parse_field_step(val, field, offset, numin, max);
          } else {
               result |= val;
          }

          if (field[*offset] == ',') {
               *offset = *offset + 1;
          } else {
               cont = false;
          }
     } while (cont);
     return result;
}

static unsigned long parse_field(const char *field, int *offset, int max, const char *fieldname) {
     unsigned long result;
     int i;
     switch (field[*offset]) {
     case '*':
          *offset = *offset + 1;
          result = 0;
          for (i = 0; i < max; i++) {
               BIT_1(result, i);
          }
          result = parse_field_step(result, field, offset, 0, max);
          break;
     default:
          result = parse_field_list(field, offset, max);
     }
     fprintf(stderr, "%s = %0lx\n", fieldname, result);
     return result;
}

static void skip_blanks(const char *field, int *offset) {
     bool_t cont = true;
     do {
          switch(field[*offset]) {
          case ' ':
          case '\t':
          case '\n':
               *offset = *offset + 1;
               break;
          default:
               cont = false;
          }
     } while(cont);
}

static yacad_cron_t impl_fn = {
     .next = (yacad_cron_next_fn)next,
     .free = (yacad_cron_free_fn)free_,
};

yacad_cron_t *yacad_cron_parse(const char *cronspec) {
     yacad_cron_impl_t *result = malloc(sizeof(yacad_cron_impl_t));
     int offset = 0;

     result->fn = impl_fn;

     skip_blanks(cronspec, &offset);
     result->spec.min = parse_field(cronspec, &offset, 60, "min");
     skip_blanks(cronspec, &offset);
     result->spec.hour = parse_field(cronspec, &offset, 24, "hour");
     skip_blanks(cronspec, &offset);
     result->spec.dom = parse_field(cronspec, &offset, 31, "dom");
     skip_blanks(cronspec, &offset);
     result->spec.mon = parse_field(cronspec, &offset, 12, "mon");
     skip_blanks(cronspec, &offset);
     result->spec.dow = parse_field(cronspec, &offset, 7, "dow");

     return I(result);
}