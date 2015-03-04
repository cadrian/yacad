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

#include <cad_array.h>
#include <cad_hash.h>
#include <cad_events.h>

#include "yacad_events.h"

typedef struct yacad_events_impl_s {
     yacad_events_t fn;
     cad_events_t *events;
     cad_array_t *on_timeout;
     cad_hash_t *on_read;
     cad_hash_t *on_write;
     cad_hash_t *on_exception;
     size_t count;
} yacad_events_impl_t;

typedef struct event_s {
     union {
          on_timeout_action timeout;
          on_event_action event;
     } action;
     void *data;
} event_t;

static unsigned int keys_hash(const int *key) {
     return (unsigned int)(*key);
}

static int keys_compare(const int *key1, const int *key2) {
     return (*key1) - (*key2);
}

static int *keys_clone(const int *key) {
     int *result = malloc(sizeof(int));
     *result = *key;
     return result;
}

static void keys_free(int *key) {
     free(key);
}

cad_hash_keys_t keys = {
     .hash = (cad_hash_keys_hash_fn)keys_hash,
     .compare = (cad_hash_keys_compare_fn)keys_compare,
     .clone = (cad_hash_keys_clone_fn)keys_clone,
     .free = (cad_hash_keys_free_fn)keys_free,
};

static void on_timeout(yacad_events_impl_t *this, on_timeout_action action, void *data) {
     event_t *event = malloc(sizeof(event_t));
     int n = this->on_timeout->count(this->on_timeout);
     event->action.timeout = action;
     event->data = data;
     this->on_timeout->insert(this->on_timeout, n, event);
     this->count++;
}

static void on_read(yacad_events_impl_t *this, on_event_action action, int fd, void *data) {
     event_t *event = malloc(sizeof(event_t));
     event->action.event = action;
     event->data = data;
     this->on_read->set(this->on_read, &fd, event);
     this->count++;
}

static void on_write(yacad_events_impl_t *this, on_event_action action, int fd, void *data) {
     event_t *event = malloc(sizeof(event_t));
     event->action.event = action;
     event->data = data;
     this->on_write->set(this->on_write, &fd, event);
     this->count++;
}

static void on_exception(yacad_events_impl_t *this, on_event_action action, int fd, void *data) {
     event_t *event = malloc(sizeof(event_t));
     event->action.event = action;
     event->data = data;
     this->on_exception->set(this->on_exception, &fd, event);
     this->count++;
}

static void clean_iterator(cad_hash_t *hash, int index, const void *key, event_t *event, yacad_events_impl_t *this) {
     free(event);
}

static void cleanup(yacad_events_impl_t *this) {
     event_t *event;
     int i, n = this->on_timeout->count(this->on_timeout);
     for (i = 0; i < n; i++) {
          event = this->on_timeout->get(this->on_timeout, i);
          free(event);
     }
     this->on_read->clean(this->on_read, (cad_hash_iterator_fn)clean_iterator, this);
     this->on_write->clean(this->on_write, (cad_hash_iterator_fn)clean_iterator, this);
     this->on_exception->clean(this->on_exception, (cad_hash_iterator_fn)clean_iterator, this);
     this->count = 0;
}

static bool_t step(yacad_events_impl_t *this) {
     bool_t result = false;

     if (this->count > 0) {
          this->events->wait(this->events, this);
          cleanup(this);
          result = true;
     }

     return result;
}

static void free_(yacad_events_impl_t *this) {
     this->events->free(this->events);
     this->on_timeout->free(this->on_timeout);
     this->on_read->free(this->on_read);
     this->on_write->free(this->on_write);
     this->on_exception->free(this->on_exception);
     free(this);
}

static yacad_events_t impl_fn = {
     .on_timeout = (yacad_events_on_timeout_fn)on_timeout,
     .on_read = (yacad_events_on_read_fn)on_read,
     .on_write = (yacad_events_on_write_fn)on_write,
     .on_exception = (yacad_events_on_exception_fn)on_exception,
     .step = (yacad_events_step_fn)step,
     .free = (yacad_events_free_fn)free_,
};

static void do_timeout(yacad_events_impl_t*this) {
     event_t *event;
     int i, n = this->on_timeout->count(this->on_timeout);
     for (i = 0; i < n; i++) {
          event = this->on_timeout->get(this->on_timeout, i);
          event->action.timeout(event->data);
     }
}

static void do_read(int fd, yacad_events_impl_t*this) {
     event_t *event = this->on_read->get(this->on_read, &fd);
     if (event != NULL) {
          event->action.event(fd, event->data);
     }
}

static void do_write(int fd, yacad_events_impl_t*this) {
     event_t *event = this->on_write->get(this->on_write, &fd);
     if (event != NULL) {
          event->action.event(fd, event->data);
     }
}

static void do_exception(int fd, yacad_events_impl_t*this) {
     event_t *event = this->on_exception->get(this->on_exception, &fd);
     if (event != NULL) {
          event->action.event(fd, event->data);
     }
}

yacad_events_t *yacad_events_new(void) {
     yacad_events_impl_t *result = malloc(sizeof(yacad_events_impl_t));
     result->fn = impl_fn;
     result->events = cad_new_events_selector(stdlib_memory);
     result->on_timeout = cad_new_array(stdlib_memory);
     result->on_read = cad_new_hash(stdlib_memory, keys);
     result->on_write = cad_new_hash(stdlib_memory, keys);
     result->on_exception = cad_new_hash(stdlib_memory, keys);
     result->events->on_timeout(result->events, (cad_on_timeout_action)do_timeout);
     result->events->on_read(result->events, (cad_on_descriptor_action)do_read);
     result->events->on_write(result->events, (cad_on_descriptor_action)do_write);
     result->events->on_exception(result->events, (cad_on_descriptor_action)do_exception);
     return &(result->fn);
}
