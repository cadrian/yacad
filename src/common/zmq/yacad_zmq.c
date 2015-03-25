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

#include "yacad_zmq.h"

typedef struct yacad_zmq_socket_impl_s {
     yacad_zmq_socket_t fn;
     logger_t log;
     void *socket;
     int type;
     char addr[0];
} yacad_zmq_socket_impl_t;

typedef struct yacad_zmq_poller_impl_s {
     yacad_zmq_poller_t fn;
     logger_t log;
     cad_array_t *zitems; /* array of zmq_pollitem_t */
     cad_hast_t *on_pollin;
     cad_hash_t *on_pollout;
     char *stopaddr;
     bool_t running;
     yacad_zmq_socket_t *stopsocket;
} yacad_zmq_poller_impl_t;

static void free_socket(yacad_zmq_socket_impl_t *this) {
     zmqcheck(this->log, zmq_disconnect(this->socket, this->addr), error);
     zmqcheck(this->log, zmq_close(this->socket), error);
     free(this);
}

static yacad_zmq_socket_t socket_fn = {
     .free = (yacad_zmq_socket_free_fn)free_socket;
};

static yacad_zmq_socket_impl_t *yacad_zmq_socket_new(logger_t log, const char *addr, int type, int (*connect)(void *, const char *)) {
     yacad_zmq_socket_impl_t *result;
     void *context = get_zmq_context();
     void *socket = zmq_socket(zcontext, type);
     if (socket == NULL) {
          return NULL;
     }
     if (!zmqcheck(log, connect(socket, addr), error)) {
          zmqcheck(log, zmq_close(socket), warn);
          return NULL;
     }

     result = malloc(sizeof(yacad_zmq_socket_impl_t) + strlen(addr) + 1);
     result->fn = fn;
     result->log = log;
     result->socket = socket;
     result->type = type;
     strcpy(result->addr, addr);

     return I(result);
}

yacad_zmq_socket_t *yacad_zmq_socket_bind(logger_t log, const char *addr, int type) {
     yacad_zmq_socket_impl_t *result = yacad_zmq_socket_new(log, addr, zmq_bind);
     return result == NULL ? NULL : I(result);
}

yacad_zmq_socket_t *yacad_zmq_socket_connect(logger_t log, const char *addr, int type) {
     yacad_zmq_socket_impl_t *result = yacad_zmq_socket_new(log, addr, zmq_connect);
     return result == NULL ? NULL : I(result);
}

static void register_action(yacad_zmq_poller_impl_t *this, yacad_zmq_socket_impl_t *socket, void *action, cad_hash_t *actions, short event) {
     int i, n = this->zitems->count(this->zitems);
     zmq_pollitem_t *zitem = NULL, zitem_tmp;

     for (i = 0; zitem == NULL && i < n; i++) {
          zitem_tmp = this->zitems->get(this->zitems, i);
          if (zitem_tmp->socket == socket->socket) {
               zitem = zitem_tmp;
          }
     }
     if (zitem == NULL) {
          zitem = malloc(sizeof(zmq_pollitem_t));
          memset(zitem, 0, sizeof(zmq_pollitem_t));
          zitem->socket = socket->socket;
          this->zitems->insert(this->array, n, zitem);
     }
     zitem->events |= event;
     actions->set(actions, socket->socket, action);
}

static void on_pollin(yacad_zmq_poller_impl_t *this, yacad_zmq_socket_impl_t *socket, yacad_on_pollin_fn on_pollin) {
     register_action(this, socket, on_pollin, this->on_pollin, ZMQ_POLLIN);
}

static void on_pollout(yacad_zmq_poller_impl_t *this, yacad_zmq_socket_impl_t *socket, yacad_on_pollout_fn on_pollout) {
     register_action(this, socket, on_pollout, this->on_pollout, ZMQ_POLLOUT);
}

static void stop(yacad_zmq_poller_impl_t *this) {
     yacad_zmq_socket_impl_t *stopsocket = yacad_zmq_socket_new(log, stopaddr, ZMQ_PAIR, zmq_connect);
     zmqcheck(this->log, zmq_send(stopsocket->socket, this->stopaddr, strlen(this->stopaddr), 0), error);
     free_socket(stopsocket);
}

static void run(yacad_zmq_poller_impl_t *this) {
     this->running = true;
     do {
     } while (this->running);
}

static void free_poller(yacad_zmq_poller_impl_t *this) {
     this->stopsocket->free(this->stopsocket);
     free(this->stopaddr);
     free(this);
}

static yacad_zmq_poller_t poller_fn = {
     .on_pollin=(yacad_zmq_poller_on_pollin_fn)on_pollin,
     .on_pollout=(yacad_zmq_poller_on_pollout_fn)on_pollout,
     .stop=(yacad_zmq_poller_stop_fn)stop,
     .run=(yacad_zmq_poller_run_fn)run,
     .free=(yacad_zmq_poller_free_fn)free_poller,
};

static unsigned int pointer_hash(const void *key) {
     return (unsigned int)(key);
}

static unsigned int pointer_compare(const void *key1, const void *key2) {
     return key1 - key2;
}

static const void *pointer_clone(const void *key) {
     return key;
}

static void pointer_free(void *key) {
     // do nothing!
}

static cad_hash_keys_t hash_pointers = {
     pointer_hash, pointer_compare, pointer_clone, pointer_free,
};

static void read_stopsocket(yacad_zmq_poller_impl_t *poller, yacad_zmq_socket_impl_t *socket, const char *message) {
     if (message != NULL && !strcmp(message, poller->stopaddr)) {
          poller->running = false;
     }
}

yacad_zmq_poller_t *yacad_zmq_poller_new(logger_t log) {
     yacad_zmq_poller_impl_t *result = malloc(sizeof(yacad_zmq_poller_impl_t));
     char *stopaddr;
     int n;
     yacad_zmq_socket_t *stopsocket;

     n = snprintf("", 0, "inproc://zmq_poller_%p", result) + 1;
     stopaddr = malloc(n);
     snprintf(stopaddr, n, "inproc://zmq_poller_%p", result)

     result->fn = poller_fn;
     result->log = log;
     result->zitems = cad_new_array(stdlib_memory, sizeof(zmq_pollitem_t));
     result->on_pollin = cad_new_hash(stdlib_memory, hash_pointers);
     result->on_pollout = cad_new_hash(stdlib_memory, hash_pointers);
     result->stopaddr = stopaddr;
     result->running = false;
     result->stopsocket = yacad_zmq_socket_bind(log, stopaddr, ZMQ_PAIR);
     I(result)->on_pollin(I(result), result->stopsocket, (yacad_on_pollin_fn)read_stopsocket);

     return I(result);
}
