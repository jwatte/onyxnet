#include "udpbase.h"

#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <onyxutil/hashtable.h>

struct udp_instance_t {
    udp_params_t *params;
    int socket;
    udp_group_t *groups;

};

udp_instance_t *udp_initialize(udp_params_t *params) {
    char port[16];
    sprintf(port, "%d", params->port);
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    addrinfo *ai = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_V4MAPPED | AI_ADDRCONFIG;
    int gaierr = getaddrinfo(udp->interface, port, );
    if (gaierr != 0) {
        params->on_error(params, UDPERR_ADDRESS_ERROR, "initialize_udp(): getaddrinfo() failed");
        return NULL;
    }

    udp_instance_t *udp = (udp_instance_t *)calloc(1, sizeof(udp_instance_t));
    if (!udp) {
        freeaddrinfo(ai);
        params->on_error(params, UDPERR_OUT_OF_MEMORY, "initialize_udp(): calloc() failed");
        return NULL;
    }

    udp->params = params;
    udp->socket = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (udp->socket < 0) {
        freeaddrinfo(ai);
        free(udp);
        params->on_error(params, UDPERR_SOCKET_ERROR, "initialize_udp(): socket() failed");
        return NULL;
    }
    if (fcntl(udp->socket, F_SETFL, fcntl(udp->socket, F_GETFL) | O_NONBLOCK) < 0) {
        freeaddrinfo(ai);
        free(udp);
        params->on_error(params, UDPERR_IO_ERROR, "initialize_udp(): fcntl() failed");
        return NULL;
    }
    if (bind(udp->socket, ai->ai_addr, ai->ai_addrlen)
        freeaddrinfo(ai);
        free(udp);
        params->on_error(params, UDPERR_SOCKET_ERROR, "initialize_udp(): bind() failed");
        return NULL;
    }

    freeaddrinfo(ai);
    return udp;
}

void udp_terminate(udp_instance_t *udp) {
    if (!udp) return;
    udp->running = false;
    if (udp->thread) {
        void *status = 0;
        pthread_join(udp->thread, &status);
    }
    close(udp->socket);
    for (
    free(udp);
}

UDPERR udp_run(udp_instance_t *instance) {
}


int udp_poll(udp_instance_t *instance) {
}


