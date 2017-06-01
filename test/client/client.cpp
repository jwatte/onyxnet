#include <onyxudp/udpclient.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>


/* This test just makes sure the client library compiles and doens't crash on start. */


void on_error(udp_client_params_t *client, UDPERR code, char const *name) {
    fprintf(stderr, "on_error: code %d: %s\n", code, name);
    assert(!"should not happen");
}

int main() {
    udp_client_params_t params;
    memset(&params, 0, sizeof(params));
    params.app_id = 123;
    params.app_version = 321;
    params.on_error = on_error;
    params.on_idle = NULL;
    params.on_payload = NULL;
    params.on_disconnect = NULL;
    udp_client_t *client = udp_client_initialize(&params);
    assert(client != NULL);
    udp_client_terminate(client);
    return 0;
}

