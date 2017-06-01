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


/* Packet format:
 *
 * All data fields are little-endian. Sorry, network gurus, but the world is 
 * little endian, and doing the work to swap the data in both sending and 
 * receiving ends (both of which run x86 or ARM in little-endian mode) makes 
 * no sense.
 *
 * <8-byte packet:
 * invalid
 *
 * 8-byte control packet:
 * crc16 (2 bytes)
 * command (2 bytes)
 * app_id (2 bytes)
 * app_version (2 bytes)
 *
 * >8-byte data packet:
 * crc32 (4 bytes)
 * app_id (2 bytes)
 * app_version (2 bytes)
 * <data> <N bytes>
 *
 * In each case, the CRC is calculated on all data following the CRC field 
 * to the end of the packet.
 *
 * For later versions of the protocol, perhaps cryptography will be added, in which 
 * case more fields will go into the header.
 */

struct command_header {
    uint16_t crc;
    uint16_t command;
    uint16_t app_id;
    uint16_t app_version;
};

struct data_header {
    uint32_t crc;
    uint16_t app_id;
    uint16_t app_version;
};

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
    if (!params->max_payload_size) {
        params->max_payload_size = UDP_DEFAULT_MAX_PAYLOAD_SIZE;
    }
    if (!params->port) {
        params->port = 4812;
    }
    char port[16];
    sprintf(port, "%d", params->port);
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    addrinfo *ai = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_V4MAPPED | AI_ADDRCONFIG;
    int gaierr = getaddrinfo(params->interface, port, &hints, &ai);
    if (gaierr != 0) {
        params->on_error(params, UDPERR_ADDRESS_ERROR, "udp_initialize(): getaddrinfo() failed");
        return NULL;
    }

    udp_instance_t *udp = (udp_instance_t *)malloc(sizeof(udp_instance_t));
    if (!udp) {
        freeaddrinfo(ai);
        params->on_error(params, UDPERR_OUT_OF_MEMORY, "udp_initialize(): malloc() failed");
        return NULL;
    }
    memset(udp, 0, sizeof(udp_instance_t));

    udp->params = params;
    udp->socket = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (udp->socket < 0) {
        freeaddrinfo(ai);
        free(udp);
        params->on_error(params, UDPERR_SOCKET_ERROR, "udp_initialize(): socket() failed");
        return NULL;
    }
    if (fcntl(udp->socket, F_SETFL, fcntl(udp->socket, F_GETFL) | O_NONBLOCK) < 0) {
        freeaddrinfo(ai);
        free(udp);
        params->on_error(params, UDPERR_IO_ERROR, "udp_initialize(): fcntl() failed");
        return NULL;
    }
    if (bind(udp->socket, ai->ai_addr, ai->ai_addrlen) < 0) {
        freeaddrinfo(ai);
        free(udp);
        params->on_error(params, UDPERR_SOCKET_ERROR, "udp_initialize(): bind() failed");
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
        udp->thread = 0;
    }

    close(udp->socket);
    udp->socket = -1;

    free(udp);
}

static void *udp_run_func(void *iptr) {
    udp_instance_t *instance = (udp_instance_t *)iptr;
    while (instance->running) {
        int n = udp_poll(instance);
        if ((n == 0) && instance->running) {
            usleep(1000);
        }
    }
    return NULL;
}

UDPERR udp_run(udp_instance_t *instance) {
    if (instance->running || instance->thread) {
        return UDPERR_INVALID_ARGUMENT;
    }
    instance->running = true;
    int i = pthread_create(&instance->thread, NULL, udp_run_func, instance);
    if (i != 0) {
        instance->running = false;
        instance->thread = 0;
        return UDPERR_IO_ERROR;
    }
    return UDP_OK;
}


int udp_poll(udp_instance_t *instance) {
    return 0;
}


