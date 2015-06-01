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

#ifndef __TEST_H__
#define __TEST_H__

#include <cad_shared.h>

/**
 * The test to implement.
 *
 * @return the test status (0 is OK)
 */
int test(void);

int __assert(int test, const char *format, ...) __PRINTF__;
void __wait_for_logger(void);

#define assert(test) do { __wait_for_logger(); result += __assert((test), "**** ASSERT FAILED %s:%d: %s\n", __FILE__, __LINE__, #test); } while(0)

/**
 * Data written in the logger
 */
char *logger_data(void);

#endif /* __TEST_H__ */
