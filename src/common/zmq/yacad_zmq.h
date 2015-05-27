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

#include "yacad.h"

#ifndef __YACAD_ZMQ_H__
#define __YACAD_ZMQ_H__

typedef struct yacad_zmq_poller_s yacad_zmq_poller_t;
typedef struct yacad_zmq_socket_s yacad_zmq_socket_t;

typedef bool_t (*yacad_on_pollin_fn)(yacad_zmq_poller_t *poller, yacad_zmq_socket_t *socket, const char *message, void *data);
typedef bool_t (*yacad_on_pollout_fn)(yacad_zmq_poller_t *poller, yacad_zmq_socket_t *socket, char * const*message, void *data);
typedef bool_t (*yacad_on_timeout_fn)(yacad_zmq_poller_t *poller, void *data);
typedef void (*yacad_timeout_fn)(yacad_zmq_poller_t *poller, struct timeval *timeout, void *data);

typedef void (*yacad_zmq_poller_on_pollin_fn)(yacad_zmq_poller_t *this, yacad_zmq_socket_t *socket, yacad_on_pollin_fn on_pollin);
typedef void (*yacad_zmq_poller_on_pollout_fn)(yacad_zmq_poller_t *this, yacad_zmq_socket_t *socket, yacad_on_pollout_fn on_pollout);
typedef void (*yacad_zmq_poller_set_timeout_fn)(yacad_zmq_poller_t *this, yacad_timeout_fn timeout, yacad_on_timeout_fn on_timeout);
typedef void (*yacad_zmq_poller_stop_fn)(yacad_zmq_poller_t *this);
typedef void (*yacad_zmq_poller_run_fn)(yacad_zmq_poller_t *this, void *data);
typedef void (*yacad_zmq_poller_free_fn)(yacad_zmq_poller_t *this);

struct yacad_zmq_poller_s {
     yacad_zmq_poller_on_pollin_fn on_pollin;
     yacad_zmq_poller_on_pollout_fn on_pollout;
     yacad_zmq_poller_set_timeout_fn set_timeout;
     yacad_zmq_poller_stop_fn stop;
     yacad_zmq_poller_run_fn run;
     yacad_zmq_poller_free_fn free;
};

yacad_zmq_poller_t *yacad_zmq_poller_new(logger_t log);

typedef void (*yacad_zmq_socket_send_fn)(yacad_zmq_socket_t *this, const char *message);
typedef void (*yacad_zmq_socket_free_fn)(yacad_zmq_socket_t *this);

struct yacad_zmq_socket_s {
     yacad_zmq_socket_send_fn send;
     yacad_zmq_socket_free_fn free;
};

yacad_zmq_socket_t *yacad_zmq_socket_bind(logger_t log, const char *addr, int type);
yacad_zmq_socket_t *yacad_zmq_socket_connect(logger_t log, const char *addr, int type);

void yacad_zmq_init(void);
void yacad_zmq_term(void);

#endif /* __YACAD_ZMQ_H__ */
