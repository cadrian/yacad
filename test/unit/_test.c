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

/**
 * @file Basic test framework.
 */

#include <stdarg.h>
#include <unistd.h>

#include "common/log/yacad_log.h"
#include "common/zmq/yacad_zmq.h"

#include "test.h"

static int output_capacity = 0;
static int output_count = 0;
static char *output_data = NULL;

static int test_logger(const char *format, ...) {
     va_list arg;
     int n;

     va_start(arg, format);
     n = vfprintf(stderr, format, arg);
     va_end(arg);

     if (n > output_capacity - ACCESS_ONCE(output_count)) {
          if (output_capacity == 0) {
               output_capacity = 4096;
               output_data = malloc(output_capacity);
          }
          while (n > output_capacity - output_count) {
               output_capacity *= 2;
          }
          output_data = realloc(output_data, output_capacity);
     }

     va_start(arg, format);
     n = vsnprintf(ACCESS_ONCE(output_data) + ACCESS_ONCE(output_count), output_capacity - output_count, format, arg);
     va_end(arg);

     output_count += n;

     return n;
}

int main(void) {
     int result;
     set_thread_name("test");
     set_logger_fn(test_logger);
     yacad_zmq_init();
     result = test();
     yacad_zmq_term();
     return result;
}

void __wait_for_logger(void) {
     volatile int count = -1;
     do {
          count = ACCESS_ONCE(output_count);
          usleep(10000);
     } while (ACCESS_ONCE(output_count) > count);
}

int __assert(int test, const char *format, ...) {
     int result = 0;
     va_list arg;
     if (!(test)) {
          va_start(arg, format);
          vfprintf(stderr, format, arg);
          va_end(arg);
          result++;
     }
     return result;
}

char *logger_data(void) {
     return ACCESS_ONCE(output_data);
}
