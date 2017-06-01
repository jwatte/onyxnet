#include "onyxudp/udpbase.h"
#include <stdio.h>
#include <assert.h>

/* this test basically just makes sure the library compiles and loads. */

void basic_error(udp_params_t *, UDPERR code, char const *text) {
    fprintf(stderr, "Error code %d: %s\n", code, text);
    assert(!"no error is allowed");
}

struct udp_params_t me = {
    4992,           //  port
    0,              //  max_payload_size
    123,            //  app_id
    1,              //  app_version
    "127.0.0.1",    //  interface
    basic_error,    //  on_error
    NULL,           //  on_idle
    NULL,           //  on_peer_new
    NULL,           //  on_peer_expired
};

int main() {
    udp_instance_t *udp = udp_initialize(&me);
    udp_terminate(udp);
    return 0;
}

