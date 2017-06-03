#include "udpclient.h"
#include "udpbase.h"
#include "protocol.h"
#include "types.h"

#include <onyxutil/vector.h>
#include <onyxutil/hashtable.h>
#include <onyxutil/crc.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>


size_t connection_hash(void const *data, size_t sz) {
    return hash_pod(data, sizeof(udp_conn_addr_t));
}

int connection_comp(void const *a, void const *b, size_t sz) {
    return memcmp(a, b, sizeof(udp_conn_addr_t));
}

static void remove_connection_from_client(udp_client_connection_t *conn) {
    int found = hash_table_remove(&conn->client->connections, conn);
    assert(found == 1);
    conn->client->params->on_disconnect(conn->client->params, conn, UDPPEER_CLIENT_DISCONNECTED);
}

static void free_client_connection(udp_client_connection_t *conn) {
    if (conn->conn_payload) {
        udp_payload_release(conn->conn_payload);
    }
    size_t n = conn->outgoing.item_count;
    for (size_t i = 0; i != n; ++i) {
        udp_payload_t *payload = (udp_payload_t *)vector_item_get(&conn->outgoing, i);
        udp_payload_release(payload);
    }
    vector_deinit(&conn->outgoing);
    memset(conn, 0xff, sizeof(*conn));
    free(conn);
}

udp_client_t *udp_client_initialize(udp_client_params_t *params) {
    if (!params->max_payload_size) {
        params->max_payload_size = UDP_DEFAULT_MAX_PAYLOAD_SIZE;
    }
    if (params->max_payload_size < UDP_MIN_PAYLOAD_SIZE) {
        params->on_error(params, UDPERR_INVALID_ARGUMENT, "udp_client_initialize(): min_payload_size too small");
        return NULL;
    }
    udp_client_t *client = (udp_client_t *)malloc(sizeof(udp_client_t));
    if (!client) {
        params->on_error(params, UDPERR_OUT_OF_MEMORY, "udp_client_initialize(): malloc() failed");
        return NULL;
    }
    memset(client, 0, sizeof(*client));
    client->params = params;
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
    hash_iterator_t iter;
    for (void *conn = hash_table_begin(&client->connections, &iter); conn; conn = hash_table_next(&iter)) {
        free_client_connection((udp_client_connection_t *)conn);
    }
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

udp_client_connection_t *udp_client_connect(udp_client_t *client, udp_conn_addr_t *addr, udp_payload_t *payload) {
    if (hash_table_find(&client->connections, addr) != NULL) {
        client->params->on_error(client->params, UDPERR_INVALID_ARGUMENT, "udp_client_connect(): already connecting to address");
        if (payload) {
            udp_payload_release(payload);
        }
        return NULL;
    }
    if (payload == NULL) {
        payload = udp_client_payload_get(client);
        if (!payload) {
            return NULL;
        }
        command_header hdr = { 0, UDP_CMD_CONNECT, client->params->app_id, client->params->app_version };
        assert(sizeof(hdr) == 6);
        hdr.crc16 = update_crc16(&hdr.command, 6, 0);
        memcpy(payload->data, &hdr, 8);
        payload->size = 8;
    }
    udp_client_connection_t *conn = (udp_client_connection_t *)malloc(sizeof(udp_client_connection_t));
    if (!conn) {
        client->params->on_error(client->params, UDPERR_OUT_OF_MEMORY, "udp_client_connect(): malloc() failed");
        udp_payload_release(payload);
        return NULL;
    }
    memset(conn, 0, sizeof(*conn));
    memcpy(&conn->addr, addr, sizeof(*addr));
    conn->client = client;
    conn->conn_payload = payload;
    if (vector_init(&conn->outgoing, sizeof(udp_payload_t *)) < 0) {
        client->params->on_error(client->params, UDPERR_OUT_OF_MEMORY, "udp_client_connect(): vector_init() failed");
        udp_payload_release(payload);
        free(conn);
        return NULL;
    }
    void *prev = hash_table_assign(&client->connections, conn);
    assert(prev == conn);
    return conn;
}

UDPERR udp_client_disconnect(udp_client_connection_t *conn) {
    command_header hdr = { 0, UDP_CMD_DISCONNECT, conn->client->params->app_id, conn->client->params->app_version };
    assert(sizeof(hdr) == 8);
    hdr.crc16 = update_crc16(&hdr.command, 6, 0);
    int i = sendto(conn->client->socket, &hdr, sizeof(hdr), 0, (sockaddr const *)&conn->addr.data[2], conn->addr.data[1]);
    if (i != sizeof(hdr)) {
        conn->client->params->on_error(conn->client->params, UDPERR_SOCKET_ERROR, "udp_client_disconnect(): sendto() failed");
    }
    remove_connection_from_client(conn);
    free_client_connection(conn);
    return UDP_OK;
}

UDPERR udp_client_payload_send(udp_client_connection_t *conn, udp_payload_t *payload) {
    return UDPERR_INVALID_ARGUMENT; // TODO
}

static void *udp_client_run_func(void *iptr) {
    udp_client_t *client = (udp_client_t *)iptr;
    while (client->running) {
        int n = udp_client_poll(client);
        if ((n == 0) && client->running) {
            usleep(1000);
        }
    }
    return NULL;
}

UDPERR udp_client_run(udp_client_t *client) {
    if (client->running || client->thread) {
        return UDPERR_INVALID_ARGUMENT;
    }
    client->running = true;
    int i = pthread_create(&client->thread, NULL, udp_client_run_func, client);
    if (i != 0) {
        client->running = false;
        client->thread = 0;
        return UDPERR_IO_ERROR;
    }
    return UDP_OK;
}

int udp_client_poll(udp_client_t *client) {
    return 0;
}

