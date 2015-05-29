#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include "common/log/yacad_log.h"

#include "_test.c"

static int output_capacity = 0;
static volatile int output_count = 0;
static char *output_data = NULL;

static int output(const char *format, ...) {
     va_list arg, zarg;
     int n;
     va_start(arg, format);
     va_copy(zarg, arg);

     n = vsnprintf("", 0, format, arg);
     if (output_capacity == 0) {
          output_capacity = 4096;
          output_data = malloc(output_capacity);
     }
     if ((n > output_capacity - output_count)) {
          do {
               output_capacity *= 2;
          } while (n > output_capacity - output_count);
          output_data = realloc(output_data, output_capacity);
     }
     n = vsnprintf(output_data + output_count, output_capacity - output_count, format, zarg);
     output_count += n;

     va_end(zarg);
     va_end(arg);

     return n;
}

int test(void) {
     volatile int count = -1;
     logger_t log = get_logger(info, output);
     log(error, "hello world");
     do {
          count = output_count;
          usleep(10000);
     } while (output_count != count);
     return strcmp(output_data + 26, " [ERROR] {test} hello world\n");
}
