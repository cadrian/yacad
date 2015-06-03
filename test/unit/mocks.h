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

#ifndef __MOCKS_H__
#define __MOCKS_H__

#include <yacad.h>

typedef bool_t (*mock_comparator_fn)(const void *expected, const void *actual);

/**
 * When preparing the tests, call the expect functions first
 */

#define expect(fun, name, value) __mock_expect(#fun, #name, sizeof(value), &(value), __mock_compare_values)
#define expect_cmp(fun, name, value, cmp) __mock_expect(#fun, #name, sizeof(value), &(value), cmp)
#define expect_string(fun, name, value) __mock_expect(#fun, #name, sizeof(value), &(value), __mock_compare_strings)
#define check(name) __mock_check(__func__, #name, sizeof(name), &(name))

/**
 * When preparing the tests, call push_result or push_void after the expects because that will "lock" a mock call.
 * In particular, take heed of push_void() for void functions
 */

#define push_result(fun, value) do {                            \
          __auto_type v = (value);                              \
          __mock_push_result(#fun, sizeof(v), &v);              \
     } while(0)
#define push_void(fun) __mock_push_result(#fun, 0, NULL)
#define result(type) (*(type*)(__mock_pop_result(__func__)))
#define no_result() ((void)(__mock_pop_result(__func__)))

bool_t __mock_compare_values(const void *expected, const void *actual);
bool_t __mock_compare_strings(const void *expected, const void *actual);

void __mock_expect(const char *fun, const char *param, size_t size, const void *expected, mock_comparator_fn cmp);
void __mock_check(const char *fun, const char *param, size_t size, const void *actual);

void __mock_push_result(const char *fun, size_t size, const void *value);
void *__mock_pop_result(const char *fun);

int verify_mocks(void);

#endif /* __MOCKS_H__ */
