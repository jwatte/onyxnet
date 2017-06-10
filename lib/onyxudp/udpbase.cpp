#include "udpbase.h"
#include "protocol.h"
#include "types.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <onyxutil/hashtable.h>
#include <onyxutil/vector.h>


udp_instance_t *udp_initialize(udp_params_t *params) {
    if (!params->max_payload_size) {
        params->max_payload_size = UDP_DEFAULT_MAX_PAYLOAD_SIZE;
    }
    if (params->max_payload_size < UDP_MIN_PAYLOAD_SIZE) {
        params->on_error(params, UDPERR_INVALID_ARGUMENT, "udp_initialize(): min_payload_size too small");
        return NULL;
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
    /* TODO: implement me */
    return 0;
}

udp_group_t *udp_group_create(udp_instance_t *instance, udp_group_params_t *params) {
    udp_group_t *ret = (udp_group_t *)malloc(sizeof(udp_group_t));
    memset(ret, 0, sizeof(*ret));
    if (vector_init(&ret->peers, sizeof(udp_peer_t *)) < 0) {
        free(ret);
        instance->params->on_error(instance->params, UDPERR_OUT_OF_MEMORY, "udp_group_create(): vector_init() failed");
        return NULL;
    }
    ret->instance = instance;
    ret->params = params;
    ret->next = instance->groups;
    return ret;
}

static void udp_peer_destroy(udp_peer_t *peer) {
    assert(peer->groups.item_count == 0);
    peer->instance->params->on_peer_expired(peer->instance->params, peer, UDPPEER_LAST_GROUP_DESTROYED);
    for (size_t i = 0, n = peer->out_queue.item_count; i != n; ++i) {
        udp_payload_t *payload = (udp_payload_t *)vector_item_get(&peer->out_queue, i);
        udp_payload_release(payload);
    }
    vector_deinit(&peer->out_queue);
    vector_deinit(&peer->groups);
    free(peer);
}

static UDPERR udp_group_peer_remove_ix(udp_group_t *group, size_t ix) {
    assert(ix < group->peers.item_count);
    if (ix >= group->peers.item_count) {
        return UDPERR_INVALID_ARGUMENT;
    }
    udp_peer_t *peer = (udp_peer_t *)vector_item_get(&group->peers, ix);
    vector_item_remove(&group->peers, ix, 1);
    group->params->on_peer_removed(group->params, peer, UDPPEER_REMOVED_FROM_GROUP);
    for (size_t i = 0, n = peer->groups.item_count; i != n; ++i) {
        udp_group_t *g = (udp_group_t *)vector_item_get(&peer->groups, i);
        if (g == group) {
            vector_item_remove(&peer->groups, i, 1);
            if (peer->groups.item_count == 0) {
                //  last group keeping peer alive
                udp_peer_destroy(peer);
            }
            return UDP_OK;
        }
    }
    //  The group was not found in the peer? Corrupt internal state.
    return UDPERR_INVALID_ARGUMENT;
}

void udp_group_destroy(udp_group_t *group) {
    udp_params_t *iparams = group->instance->params;
    //  for each peer
    int n_errors = 0;
    for (size_t i = group->peers.item_count; i > 0; --i) {
        //  remove peer from group
        UDPERR err = udp_group_peer_remove_ix(group, i-1);
        if (err != UDP_OK) {
            ++n_errors;
        }
    }
    //  remove group from instance
    //  assume not found
    n_errors++;
    for (udp_group_t **gp = &group->instance->groups; *gp; gp = &(*gp)->next) {
        if (*gp == group) {
            *gp = group->next;
            //  found it
            n_errors--;
            break;
        }
    }
    //  free memory
    vector_deinit(&group->peers);
    free(group);
    if (n_errors > 0) {
        //  This is a lame error message, but better than a poke in the eye.
        //  This should never happen, so at least it will provide an indication 
        //  to go look at how the group got corrupted.
        iparams->on_error(iparams, UDPERR_INVALID_ARGUMENT, "udp_group_destroy(): errors during group destruction");
    }
}

UDPERR udp_group_peer_remove(udp_group_t *group, udp_peer_t *peer) {
    for (size_t i = 0, n = group->peers.item_count; i != n; ++i) {
        udp_peer_t *gp = (udp_peer_t *)vector_item_get(&group->peers, i);
        if (gp == peer) {
            udp_group_peer_remove_ix(group, i);
            return UDP_OK;
        }
    }
    return UDPERR_INVALID_ARGUMENT;
}

UDPERR udp_group_peer_add(udp_group_t *group, udp_peer_t *peer) {
    for (size_t i = 0, n = peer->groups.item_count; i != n; ++i) {
        udp_group_t *gp = (udp_group_t *)vector_item_get(&peer->groups, i);
        if (gp == group) {
            //  already in the group
            return UDPERR_INVALID_ARGUMENT;
        }
    }
    size_t i = vector_item_append(&peer->groups, &group);
    if (i != 0) {
        size_t j = vector_item_append(&group->peers, &peer);
        if (j == 0) {
            vector_item_remove(&peer->groups, i - 1, 1);
            return UDPERR_OUT_OF_MEMORY;
        }
    }
    return UDP_OK;
}

