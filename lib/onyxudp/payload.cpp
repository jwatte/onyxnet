#include "udpbase.h"
#include "udpclient.h"
#include "types.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>


udp_payload_t *udp_payload_new(size_t size, udp_params_t *server, udp_client_params_t *client) {
    char *ret = (char *)malloc(sizeof(udp_payload_t) + sizeof(udp_payload_owner_t) + size);
    memset(ret, 0, sizeof(udp_payload_t) + sizeof(udp_payload_owner_t));
    udp_payload_t *pl = (udp_payload_t *)ret;
    pl->data = ret + sizeof(udp_payload_t) + sizeof(udp_payload_owner_t);
    pl->size = 0;
    pl->_refcount = 1;
    pl->app_id = 0;
    pl->app_version = 0;
    udp_payload_owner_t *owner = (udp_payload_owner_t *)(ret + sizeof(udp_payload_t));
    owner->server = server;
    owner->client = client;
    return pl;
}

udp_payload_t *udp_payload_get(udp_instance_t *instance) {
    return udp_payload_new(instance->params->max_payload_size, instance->params, NULL);
}

udp_payload_t *udp_client_payload_get(udp_client_t *client) {
    return udp_payload_new(client->params->max_payload_size, NULL, client->params);
}

void udp_payload_release(udp_payload_t *payload) {
    if (payload->_refcount == 0) {
        udp_payload_owner_t *owner = (udp_payload_owner_t *)(payload + 1);
        if (owner->server) {
            owner->server->on_error(owner->server, UDPERR_INVALID_ARGUMENT, "udp_payload_release(): invalid payload (server)");
        } else {
            owner->client->on_error(owner->client, UDPERR_INVALID_ARGUMENT, "udp_payload_release(): invalid payload (client)");
        }
        return;
    }
    /* payloads with a refcount of 0xffff are "pinned" and live forever */
    if (payload->_refcount < 0xffff) {
        --payload->_refcount;
        if (payload->_refcount == 0) {
            memset(payload, 0xff, sizeof(*payload));
            ::free(payload);
        }
    }
}

void udp_payload_hold(udp_payload_t *payload) {
    udp_payload_owner_t *owner = (udp_payload_owner_t *)(payload + 1);
    if (payload->_refcount == 0) {
        if (owner->server) {
            owner->server->on_error(owner->server, UDPERR_INVALID_ARGUMENT, "udp_payload_hold(): invalid payload (server)");
        } else {
            owner->client->on_error(owner->client, UDPERR_INVALID_ARGUMENT, "udp_payload_hold(): invalid payload (client)");
        }
        return;
    }
    if (payload->_refcount < 0xffff) {
        ++payload->_refcount;
    } else {
        if (owner->server) {
            owner->server->on_error(owner->server, UDPERR_INVALID_ARGUMENT, "udp_payload_hold(): payload refcount pinned (server)");
        } else {
            owner->client->on_error(owner->client, UDPERR_INVALID_ARGUMENT, "udp_payload_hold(): payload refcount pinned (client)");
        }
    }
}


