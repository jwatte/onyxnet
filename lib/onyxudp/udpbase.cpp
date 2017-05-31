#include "udpbase.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <onyxutil/hashtable.h>
#include <onyxutil/vector.h>

struct udp_instance_t {
    udp_params_t *params;
    udp_group_t *groups;
    int socket;
    int running;
    pthread_t thread;
    hash_table_t peers;
};

struct udp_group_t {
    udp_instance_t *instance;
    udp_group_params_t *params;
    udp_group_t *next;
    vector_t peers;
};

struct udp_peer_t {
    udp_instance_t *instance;
    uint64_t last_timestamp;
    vector_t out_queue;
    vector_t groups;
    udp_addr_t address;
};

udp_instance_t *udp_initialize(udp_params_t *params) {
    char port[16];
    sprintf(port, "%d", params->port);
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    addrinfo *ai = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_V4MAPPED | AI_ADDRCONFIG;
    int gaierr = getaddrinfo(params->interface, port, &hints, &ai);
    if (gaierr != 0) {
        params->on_error(params, UDPERR_ADDRESS_ERROR, "initialize_udp(): getaddrinfo() failed");
        return NULL;
    }

    udp_instance_t *udp = (udp_instance_t *)malloc(sizeof(udp_instance_t));
    memset(udp, 0, sizeof(udp_instance_t));
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
    if (bind(udp->socket, ai->ai_addr, ai->ai_addrlen) < 0) {
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

    free(udp);
}

UDPERR udp_run(udp_instance_t *instance) {
    if (instance->running || instance->thread) {
        return UDPERR_INVALID_ARGUMENT;
    }
    instance->running = true;
    return UDP_OK;
}


int udp_poll(udp_instance_t *instance) {
    return 0;
}


