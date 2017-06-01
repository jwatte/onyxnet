#include "udpclient.h"
#include "udpbase.h"

#include <onyxutil/vector.h>
#include <onyxutil/hashtable.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>


struct udp_client_t {
    udp_client_params_t params;
    int socket;
    int running;
    pthread_t thread;
    hash_table_t connections;
};

struct udp_client_connection_t {
    udp_conn_addr_t addr;
    udp_client_t *client;
    udp_payload_t *conn_payload;
    uint64_t last_transmit;
    uint64_t last_receive;
};

size_t connection_hash(void const *data, size_t sz) {
    return hash_pod(data, sizeof(udp_conn_addr_t));
}

int connection_comp(void const *a, void const *b, size_t sz) {
    return memcmp(a, b, sizeof(udp_conn_addr_t));
}

udp_client_t *udp_client_initialize(udp_client_params_t *params) {
    if (!params->max_payload_size) {
        params->max_payload_size = UDP_DEFAULT_MAX_PAYLOAD_SIZE;
    }
    udp_client_t *client = (udp_client_t *)malloc(sizeof(udp_client_t));
    if (!client) {
        params->on_error(params, UDPERR_OUT_OF_MEMORY, "udp_client_initialize(): malloc() failed");
        return NULL;
    }
    memset(client, 0, sizeof(*client));
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    addrinfo *ai = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV | AI_V4MAPPED | AI_ADDRCONFIG;
    int gaierr = getaddrinfo(NULL, "0", &hints, &ai);
    if (gaierr != 0) {
        params->on_error(params, UDPERR_ADDRESS_ERROR, "udp_client_initialize(): getaddrinfo() failed");
        free(client);
        return NULL;
    }
    client->socket = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (client->socket < 0) {
        params->on_error(params, UDPERR_SOCKET_ERROR, "udp_client_initialize(): socket() failed");
        freeaddrinfo(ai);
        free(client);
        return NULL;
    }
    if (ai->ai_family == AF_INET6) {
        int off = 0;
        ::setsockopt(client->socket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&off, sizeof(off));
    }
    freeaddrinfo(ai);
    hash_table_t *ok = hash_table_init(
            &client->connections,
            sizeof(udp_client_connection_t),
            HASHTABLE_POINTERS,
            connection_hash,
            connection_comp
            );
    if (!ok) {
        params->on_error(params, UDPERR_OUT_OF_MEMORY, "udp_client_initialize(): hash_table_init() failed");
        free(client);
        return NULL;
    }
    return client;
}

void udp_client_terminate(udp_client_t *client) {
    hash_table_deinit(&client->connections);
    close(client->socket);
    memset(client, 0xff, sizeof(*client));
    free(client);
}

UDPERR udp_client_address_resolve(udp_addr_t *in_addr, udp_conn_addr_t *out_addr) {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    addrinfo *ai = 0;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
    int gaierr = getaddrinfo(in_addr->addr, in_addr->port[0] ? in_addr->port : "4812", &hints, &ai);
    if (gaierr != 0) {
        return UDPERR_ADDRESS_ERROR;
    }
    if (ai->ai_addrlen > sizeof(udp_conn_addr_t)-2) {
        freeaddrinfo(ai);
        return UDPERR_ADDRESS_ERROR;
    }
    memset(out_addr, 0, sizeof(*out_addr));
    out_addr->data[0] = 1;
    out_addr->data[1] = ai->ai_addrlen;
    memcpy(&out_addr->data[2], ai->ai_addr, ai->ai_addrlen);
    freeaddrinfo(ai);
    return UDP_OK;
}

/*
udp_client_connection_t *udp_client_connect(udp_client_t *client, udp_conn_addr_t *addr, udp_payload_t *payload) {
}

UDPERR udp_client_disconnect(udp_client_connection_t *conn) {
}

UDPERR udp_client_payload_send(udp_client_connection_t *conn, udp_payload_t *payload) {
}
*/
