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

static bool_t __zmqcheck(logger_t log, int zmqerr, level_t level, const char *zmqaction, const char *file, unsigned int line);
#define zmqcheck(log, zmqaction, level) __zmqcheck(log, (zmqaction), (level), #zmqaction, __FILE__, __LINE__)

static void *zmq_context = NULL;

void yacad_zmq_init(void) {
     zmq_context = zmq_ctx_new();
     if (zmq_context == NULL) {
          fprintf(stderr, "**** ERROR: could not create 0MQ context\n");
          exit(1);
     }
}

void yacad_zmq_term(void) {
     zmqcheck(NULL, zmq_ctx_term(zmq_context), error);
     zmq_context = NULL;
}

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
     cad_hash_t *sockets;
     cad_hash_t *on_pollin;
     cad_hash_t *on_pollout;
     yacad_timeout_fn timeout;
     yacad_on_timeout_fn on_timeout;
     char *stopaddr;
     bool_t running;
     yacad_zmq_socket_t *stopsocket;
} yacad_zmq_poller_impl_t;

static void send(yacad_zmq_socket_impl_t *this, const char *message) {
     zmqcheck(this->log, zmq_send(this->socket, message, strlen(message), 0), warn);
}

static void free_socket(yacad_zmq_socket_impl_t *this) {
     zmqcheck(this->log, zmq_disconnect(this->socket, this->addr), error);
     zmqcheck(this->log, zmq_close(this->socket), error);
     free(this);
}

static yacad_zmq_socket_t socket_fn = {
     .send = (yacad_zmq_socket_send_fn)send,
     .free = (yacad_zmq_socket_free_fn)free_socket,
};

static yacad_zmq_socket_impl_t *yacad_zmq_socket_new(logger_t log, const char *addr, int type, int (*connect)(void *, const char *)) {
     yacad_zmq_socket_impl_t *result = NULL;
     void *socket = zmq_socket(zmq_context, type);
     if (socket != NULL) {
          if (!zmqcheck(log, connect(socket, addr), error)) {
               zmqcheck(log, zmq_close(socket), warn);
          } else {
               result = malloc(sizeof(yacad_zmq_socket_impl_t) + strlen(addr) + 1);
               result->fn = socket_fn;
               result->log = log;
               result->socket = socket;
               result->type = type;
               strcpy(result->addr, addr);
          }
     }
     return result;
}

yacad_zmq_socket_t *yacad_zmq_socket_bind(logger_t log, const char *addr, int type) {
     yacad_zmq_socket_impl_t *result = yacad_zmq_socket_new(log, addr, type, zmq_bind);
     return result == NULL ? NULL : I(result);
}

yacad_zmq_socket_t *yacad_zmq_socket_connect(logger_t log, const char *addr, int type) {
     yacad_zmq_socket_impl_t *result = yacad_zmq_socket_new(log, addr, type, zmq_connect);
     return result == NULL ? NULL : I(result);
}

static void register_action(yacad_zmq_poller_impl_t *this, yacad_zmq_socket_impl_t *socket, void *action, cad_hash_t *actions, short event) {
     int i, n = this->zitems->count(this->zitems);
     zmq_pollitem_t *zitem = NULL, *zitem_tmp;
     zmq_pollitem_t zitem_new;

     for (i = 0; zitem == NULL && i < n; i++) {
          zitem_tmp = this->zitems->get(this->zitems, i);
          if (zitem_tmp->socket == socket->socket) {
               zitem = zitem_tmp;
          }
     }
     if (zitem == NULL) {
          memset(&zitem_new, 0, sizeof(zmq_pollitem_t));
          zitem_new.socket = socket->socket;
          zitem = this->zitems->insert(this->zitems, n, &zitem_new);
     }

     zitem->events |= event;
     actions->set(actions, socket->socket, action);
     this->sockets->set(this->sockets, socket->socket, socket);
}

static void on_pollin(yacad_zmq_poller_impl_t *this, yacad_zmq_socket_impl_t *socket, yacad_on_pollin_fn on_pollin) {
     register_action(this, socket, on_pollin, this->on_pollin, ZMQ_POLLIN);
}

static void on_pollout(yacad_zmq_poller_impl_t *this, yacad_zmq_socket_impl_t *socket, yacad_on_pollout_fn on_pollout) {
     register_action(this, socket, on_pollout, this->on_pollout, ZMQ_POLLOUT);
}

static void set_timeout(yacad_zmq_poller_impl_t *this, yacad_timeout_fn timeout, yacad_on_timeout_fn on_timeout) {
     this->timeout = timeout;
     this->on_timeout = on_timeout;
}

static void stop(yacad_zmq_poller_impl_t *this) {
     yacad_zmq_socket_impl_t *stopsocket = yacad_zmq_socket_new(this->log, this->stopaddr, ZMQ_PAIR, zmq_connect);
     zmqcheck(this->log, zmq_send(stopsocket->socket, this->stopaddr, strlen(this->stopaddr), 0), error);
     free_socket(stopsocket);
}

static void clean_nothing(void *hash, int index, const void *key, void *value, void *data) {
}

static void run(yacad_zmq_poller_impl_t *this, void *data) {
     zmq_pollitem_t *zitems = this->zitems->get(this->zitems, 0);
     int i, zn = this->zitems->count(this->zitems);
     struct timeval now, tmout;
     long timeout;
     yacad_on_pollin_fn on_pollin;
     yacad_on_pollout_fn on_pollout;
     bool_t found;
     int n, c = 0;
     char *strmsgin = NULL;
     const char *strmsgout;
     zmq_msg_t msg;
     yacad_zmq_socket_t *socket;
     bool_t r;

     this->running = true;
     do {
          if (this->timeout == NULL) {
               timeout = -1;
          } else {
               gettimeofday(&now, NULL);
               this->timeout(I(this), &tmout, data);
               timeout = 1000L * (tmout.tv_sec - now.tv_sec);
               if (timeout < 0) {
                    timeout = 0;
               }
          }

          if (!zmqcheck(this->log, zmq_poll(zitems, zn, timeout), debug)) {
               this->running = false;
          } else {
               found = false;
               r = true;
               for (i = 0; i < zn; i++) {

                    if (zitems[i].revents & ZMQ_POLLIN) {
                         if (!zmqcheck(this->log, zmq_msg_init(&msg), error) ||
                             !zmqcheck(this->log, n = zmq_msg_recv(&msg, zitems[i].socket, 0), error)) {
                              this->running = false;
                         } else {
                              if (n > 0) {
                                   if (c < n) {
                                        if (c == 0) {
                                             strmsgin = malloc(c = 4096);
                                        } else {
                                             do {
                                                  c *= 2;
                                             } while (c < n);
                                             strmsgin = realloc(strmsgin, c);
                                        }
                                   }
                                   memcpy(strmsgin, zmq_msg_data(&msg), n);
                                   zmqcheck(this->log, zmq_msg_close(&msg), warn);
                                   strmsgin[n] = '\0';
                                   on_pollin = this->on_pollin->get(this->on_pollin, zitems[i].socket);
                                   if (on_pollin == NULL) {
                                        this->log(warn, "No pollin callback, lost message: %s", strmsgin);
                                   } else {
                                        socket = this->sockets->get(this->sockets, zitems[i].socket);
                                        r = on_pollin(I(this), socket, strmsgin, data);
                                        this->running &= r;
                                   }
                              }
                         }
                         found = true;
                    }

                    if (zitems[i].revents & ZMQ_POLLOUT) {
                         on_pollout = this->on_pollout->get(this->on_pollout, zitems[i].socket);
                         if (on_pollout == NULL) {
                              this->log(warn, "No pollout callback");
                         } else {
                              socket = this->sockets->get(this->sockets, zitems[i].socket);
                              r = on_pollout(I(this), socket, (char * const *)&strmsgout, data);
                              this->running &= r;
                              if (strmsgout != NULL) {
                                   zmqcheck(this->log, zmq_send(zitems[i].socket, strmsgout, strlen(strmsgout), 0), warn);
                              }
                         }
                         found = true;
                    }
               }

               if (!found && this->on_timeout != NULL) {
                    this->running = this->on_timeout(I(this), data);
               }
          }
     } while (this->running);

     this->zitems->clear(this->zitems);
     this->sockets->clean(this->sockets, clean_nothing, NULL);
     this->on_pollin->clean(this->on_pollin, clean_nothing, NULL);
     this->on_pollout->clean(this->on_pollout, clean_nothing, NULL);
}

static void free_poller(yacad_zmq_poller_impl_t *this) {
     this->zitems->free(this->zitems);
     this->sockets->free(this->sockets);
     this->on_pollin->free(this->on_pollin);
     this->on_pollout->free(this->on_pollout);
     this->stopsocket->free(this->stopsocket);
     free(this->stopaddr);
     free(this);
}

static yacad_zmq_poller_t poller_fn = {
     .on_pollin=(yacad_zmq_poller_on_pollin_fn)on_pollin,
     .on_pollout=(yacad_zmq_poller_on_pollout_fn)on_pollout,
     .set_timeout = (yacad_zmq_poller_set_timeout_fn)set_timeout,
     .stop=(yacad_zmq_poller_stop_fn)stop,
     .run=(yacad_zmq_poller_run_fn)run,
     .free=(yacad_zmq_poller_free_fn)free_poller,
};

static unsigned int pointer_hash(const void *key) {
     return (unsigned int)(key - NULL);
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
     .hash = (cad_hash_keys_hash_fn)pointer_hash,
     .compare = (cad_hash_keys_compare_fn)pointer_compare,
     .clone = (cad_hash_keys_clone_fn)pointer_clone,
     .free = (cad_hash_keys_free_fn)pointer_free,
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

     n = snprintf("", 0, "inproc://zmq_poller_%p", result) + 1;
     stopaddr = malloc(n);
     snprintf(stopaddr, n, "inproc://zmq_poller_%p", result);

     result->fn = poller_fn;
     result->log = log;
     result->zitems = cad_new_array(stdlib_memory, sizeof(zmq_pollitem_t));
     result->sockets = cad_new_hash(stdlib_memory, hash_pointers);
     result->on_pollin = cad_new_hash(stdlib_memory, hash_pointers);
     result->on_pollout = cad_new_hash(stdlib_memory, hash_pointers);
     result->stopaddr = stopaddr;
     result->timeout = NULL;
     result->running = false;
     result->stopsocket = yacad_zmq_socket_bind(log, stopaddr, ZMQ_PAIR);
     I(result)->on_pollin(I(result), result->stopsocket, (yacad_on_pollin_fn)read_stopsocket);

     return I(result);
}

static bool_t __zmqcheck(logger_t log, int zmqerr, level_t level, const char *zmqaction, const char *file, unsigned int line) {
     int result = true;
     char *paren;
     int len, err;
     if (zmqerr == -1) {
          result = false;
          if (log != NULL) {
               err = zmq_errno();
               paren = strchrnul(zmqaction, '(');
               len = paren - zmqaction;
               log(debug, "Error %d in %s line %u: %s", err, file, line, zmqaction);
               log(level, "%.*s: error: %s", len, zmqaction, zmq_strerror(err));
          }
     }
     return result;
}
