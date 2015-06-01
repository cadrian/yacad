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

#include "common/cron/yacad_cron.h"

/**
 * Always gets the same "current minute"
 */
static struct tm mock_get_current_minute_fn(void) {
     time_t t;
     static struct tm result = {
          .tm_sec = 0,
          .tm_min = 1,
          .tm_hour = 1,
          .tm_mday = 1,
          .tm_mon = 1,
          .tm_year = 115
     };
     static int init = 0;
     if (!init) {
          /* fix the struct to fill all the fields */
          t = mktime(&result);
          localtime_r(&t, &result);
          init = 1;
     }
     return result;
}

static int run_test(const char *format, int min, int hour, int mday, int mon, int year) {
     yacad_cron_t *cron;
     time_t t;
     static logger_t log = NULL;
     struct tm expected = {
          .tm_sec = 0,
          .tm_min = min,
          .tm_hour = hour,
          .tm_mday = mday,
          .tm_mon = mon,
          .tm_year = year - 1900,
     };
     struct tm actual;
     struct timeval tv;

     if (log == NULL) {
          log = get_logger(info);
     }

     t = mktime(&expected);
     localtime_r(&t, &expected);

     cron = yacad_cron_parse(log, format);

     tv = cron->next(cron);
     localtime_r(&tv.tv_sec, &actual);

     log(trace, "#### %d,%d,%d,%d,%d / %d,%d,%d,%d,%d ####\n",
         expected.tm_min, expected.tm_hour, expected.tm_mday, expected.tm_mon, expected.tm_year,
         actual.tm_min, actual.tm_hour, actual.tm_mday, actual.tm_mon, actual.tm_year);

     return expected.tm_min == actual.tm_min
          && expected.tm_hour == actual.tm_hour
          && expected.tm_mday == actual.tm_mday
          && expected.tm_mon == actual.tm_mon
          && expected.tm_year == actual.tm_year;
}

int test(void) {
     int result = 0;
     set_get_current_minute_fn(mock_get_current_minute_fn);

     assert(run_test("* * * * *", 2, 1, 1, 1, 2015));
     assert(run_test("10 * * * *", 10, 1, 1, 1, 2015));
     assert(run_test("1 * * * *", 1, 2, 1, 1, 2015));

     fprintf(stderr, "Done.\n");

     return result;
}
