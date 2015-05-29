#include "common/zmq/yacad_zmq.h"

int test(void);

int main(void) {
     int result;
     set_thread_name("test");
     yacad_zmq_init();
     result = test();
     yacad_zmq_term();
     return result;
}
