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

#include "mocks.h"

#include <string.h>

#include <cad_array.h>
#include <cad_hash.h>

typedef struct mock_param_s {
     mock_comparator_fn cmp;
     size_t size;
     char data[0];
} mock_param_t;

typedef struct mock_result_s {
     size_t size;
     char data[0];
} mock_result_t;

typedef struct mock_s {
     mock_result_t *result;
     bool_t returned;
     cad_hash_t *params;
} mock_t;

static cad_hash_t *mocks = NULL;
static int failed_mocks_count = 0;

static void mock_init(void) {
     if (mocks == NULL) {
          mocks = cad_new_hash(stdlib_memory, cad_hash_strings);
     }
}

static mock_t *get_mock_expectation(const char *fun) {
     cad_array_t *mocklist;
     static mock_t template = {NULL, false, NULL};
     mock_t *mock;

     mock_init();
     mocklist = (cad_array_t *)mocks->get(mocks, fun);
     if (mocklist == NULL) {
          mocklist = cad_new_array(stdlib_memory, sizeof(mock_t));
          mocks->set(mocks, fun, mocklist);
     }

     if (mocklist->count(mocklist) > 0) {
          mock = mocklist->get(mocklist, 0);
     }
     if (mock->result != NULL) {
          mock = mocklist->insert(mocklist, 0, &template);
     }

     return mock;
}

void __mock_push_result(const char *fun, size_t size, const void *value) {
     mock_t *mock = get_mock_expectation(fun);
     mock_result_t *result = malloc(sizeof(mock_result_t) + size);
     result->size = size;
     memcpy(result->data, value, size);
     mock->result = result;
}

static void param_cleaner(void *hash, int index, const void *key, void *value, void *data) {
     free(value);
}

static mock_t *get_mock_check(const char *fun) {
     cad_array_t *mocklist;
     mock_t *mock, *result = NULL;
     int n;

     if (mocks != NULL) {
          mocklist = (cad_array_t *)mocks->get(mocks, fun);
          if (mocklist != NULL) {
               for (n = mocklist->count(mocklist); n > 0 && result == NULL; n--) {
                    mock = mocklist->get(mocklist, n - 1);
                    if (mock->returned) {
                         mocklist->del(mocklist, n - 1);
                         free(mock->result);
                         if (mock->params != NULL) {
                              mock->params->clean(mock->params, param_cleaner, NULL);
                              mock->params->free(mock->params);
                         }
                         free(mock);
                    } else {
                         result = mock;
                    }
               }
          }
     }

     return result;
}

void *__mock_pop_result(const char *fun) {
     void *result = NULL;
     mock_t *mock = get_mock_check(fun);

     if (mock == NULL) {
          fprintf(stderr, "**** Result not found for function %s\n", fun);
          failed_mocks_count++;
     } else {
          mock->returned = true;
          result = mock->result->data;
     }

     return result;
}

bool_t __mock_same_values(const void *expected, const void *actual, size_t size) {
     return !memcmp(expected, actual, size);
}

bool_t __mock_same_strings(const void *expected, const void *actual, size_t size) {
     char **e = (char**)expected;
     char **a = (char**)actual;
     return actual != NULL && !strcmp(*e, *a);
}

bool_t __mock_different_values(const void *expected, const void *actual, size_t size) {
     return !!memcmp(expected, actual, size);
}

void __mock_expect(const char *fun, const char *param, size_t size, const void *expected, mock_comparator_fn cmp) {
     mock_t *mock = get_mock_expectation(fun);
     mock_param_t *value = malloc(sizeof(mock_param_t) + size);

     value->cmp = cmp;
     value->size = size;
     memcpy(value->data, expected, size);

     if (mock->params == NULL) {
          mock->params = cad_new_hash(stdlib_memory, cad_hash_strings);
     }

     mock->params->set(mock->params, param, value);
}

static char *content(const void *data, size_t size) {
     static char *result = NULL;
     static int capacity = 0;
     char *fmt;
     const char *value = (const char *)data;
     int i, n, count;
     if (capacity == 0) {
          result = malloc(capacity = 4096);
     }
     count = 0;
     fmt = "[%02x";
     for (i = 0; i < size; i++) {
          n = snprintf("", 0, fmt, value[i]);
          if (n > capacity - count - 2) {
               do {
                    capacity *= 2;
               } while (n > capacity - count);
               result = realloc(result, capacity);
          }
          n = snprintf(result + count, capacity - count, fmt, value[i]);
          count += n;
          fmt = " %02x";
     }
     snprintf(result + count, capacity - count, "]");
     return result;
}

void __mock_check(const char *fun, const char *param, size_t size, const void *actual) {
     mock_t *mock = get_mock_check(fun);
     mock_param_t *value;
     bool_t checked = false;

     if (mock != NULL && mock->params != NULL) {
          value = mock->params->get(mock->params, param);
          if (value != NULL) {
               if (!value->cmp(value->data, actual, size)) {
                    fprintf(stderr, "**** Value mismatch for %s [%lu] in %s\n", param, (unsigned long)size, fun);
                    fprintf(stderr, "     - expected: %s\n", content(value->data, size));
                    fprintf(stderr, "     - actual:   %s\n", content(actual, size));
                    failed_mocks_count++;
               }
               checked = true;
          }
     }

     if (!checked) {
          fprintf(stderr, "**** Check value not set for %s in %s\n", param, fun);
          failed_mocks_count++;
     }
}

int verify_mocks(void) {
     int result = failed_mocks_count;
     failed_mocks_count = 0;
     return result;
}
