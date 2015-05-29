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

/**
 * The test to implement.
 *
 * @return the test status (0 is OK)
 */
int test(void);

int main(void) {
     int result;
     set_thread_name("test");
     yacad_zmq_init();
     result = test();
     yacad_zmq_term();
     return result;
}

static int output_capacity = 0;
static volatile int output_count = 0;
static char *output_data = NULL;

int test_logger(const char *format, ...) {
     va_list arg, zarg;
     int n;
     va_start(arg, format);
     va_copy(zarg, arg);

     n = vsnprintf("", 0, format, arg) + 1;
     if (n > output_capacity - output_count) {
          if (output_capacity == 0) {
               output_capacity = 4096;
               output_data = malloc(output_capacity);
          }
          while (n > output_capacity - output_count) {
               output_capacity *= 2;
          }
          output_data = realloc(output_data, output_capacity);
     }
     n = vsnprintf(output_data + output_count, output_capacity - output_count, format, zarg);
     output_count += n;

     va_end(zarg);
     va_end(arg);

     return n;
}

void wait_for_logger(void) {
     volatile int count = -1;
     do {
          count = output_count;
          usleep(10000);
     } while (output_count != count);
}

#define assert(test) do if (!(test)) {                                  \
               fprintf(stderr, "**** ASSERT FAILED: %s\n", #test);      \
               result++;                                                \
          } while(0)
