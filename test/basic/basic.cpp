#include "udpbase.h"
#include <stdio.h>

void basic_error(udp_params_t *, int code, char const *text) {
    fprintf(stderr, "Error code %d: %s\n", code, text);
}

struct udp_params_t me = {
    4992,           //  port
    basic_error,    //  error
};

int main() {
    udp_instance_t *udp = initialize_udp(&me);
    terminate_udp(udp);
    return 0;
}

