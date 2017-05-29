#include "udpbase.h"

#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


struct udp_instance_t {
    udp_params_t *params;
    int socket;
};

udp_instance_t *initialize_udp(udp_params_t *params) {
    udp_instance_t *udp = (udp_instance_t *)calloc(1, sizeof(udp_instance_t));
    if (!udp) {
        params->error(params, NET_OUT_OF_MEMORY, "initialize_udp(): callc() failed");
        return NULL;
    }
    udp->params = params;
    udp->socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp->socket < 0) {
        free(udp);
        params->error(params, NET_SOCKET_ERROR, "initialize_udp(): socket() failed");
        return NULL;
    }
    if (fcntl(udp->socket, F_SETFL, fcntl(udp->socket, F_GETFL) | O_NONBLOCK) < 0) {
        free(udp);
        params->error(params, NET_IO_ERROR, "initialize_udp(): fcntl() failed");
        return NULL;
    }
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(params->port);
    if (bind(udp->socket, (sockaddr *)&sin, sizeof(sin)) < 0) {
        free(udp);
        params->error(params, NET_IO_ERROR, "initialize_udp(): bind() failed");
        return NULL;
    }

    return udp;
}

void terminate_udp(udp_instance_t *udp) {
    if (!udp) return;
    close(udp->socket);
    free(udp);
}

