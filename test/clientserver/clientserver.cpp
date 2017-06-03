#include <onyxudp/udpbase.h>
#include <onyxudp/udpclient.h>
#include <onyxutil/vector.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>


struct server {
    udp_params_t params;
    int num_errors;
    int num_idles;
    int num_peers_new;
    int num_peers_expired;
    int num_peer_messages;
    int num_peers_removed;
    udp_instance_t *instance;
    udp_group_params_t gp1;
    server *self1;
    udp_group_t *group1;
    udp_group_params_t gp2;
    server *self2;
    udp_group_t *group2;
    udp_group_params_t gp3;
    server *self3;
    udp_group_t *group3;
    vector_t packets;
};

struct client {
    udp_client_params_t params;
    int num_errors;
    int num_idles;
    int num_payloads;
    int num_disconnects;
    udp_client_t *client;
    vector_t packets;
};

server server1;
client client1;
client client2;

void on_peer_message(udp_group_params_t *gpar, udp_peer_t *peer, udp_payload_t *payload) {
    server **spp = (server **)(gpar + 1);
    (*spp)->num_peer_messages++;
}

void on_peer_removed(udp_group_params_t *gpar, udp_peer_t *peer, UDPPEER reason) {
    server **spp = (server **)(gpar + 1);
    (*spp)->num_peers_removed++;
}

void on_error(udp_params_t *params, UDPERR err, char const *text) {
    fprintf(stderr, "SERVER ERROR: %d (%s)\n", err, text);
    ((server *)params)->num_errors++;
}

void on_idle(udp_params_t *params) {
    ((server *)params)->num_idles++;
}

void on_peer_new(udp_params_t *params, udp_peer_t *peer, udp_payload_t *payload) {
    server *srv = (server *)params;
    srv->num_peers_new++;
    if (!srv->group1) {
        srv->gp1.on_peer_message = on_peer_message;
        srv->gp1.on_peer_removed = on_peer_removed;
        srv->self1 = srv;
        srv->group1 = udp_group_create(srv->instance, &srv->gp1);
        assert(srv->group1 != NULL);
    }
    /* everybody is added to group 1 */
    UDPERR err = udp_group_peer_add(srv->group1, peer);
    assert(err == UDP_OK);
    if (!srv->group2) {
        srv->gp2.on_peer_message = on_peer_message;
        srv->gp2.on_peer_removed = on_peer_removed;
        srv->self2 = srv;
        srv->group2 = udp_group_create(srv->instance, &srv->gp2);
        assert(srv->group2 != NULL);
        /* only client 1 is added to group 2*/
        err = udp_group_peer_add(srv->group2, peer);
        assert(err == UDP_OK);
    } else {
        if (!srv->group3) {
            srv->gp3.on_peer_message = on_peer_message;
            srv->gp3.on_peer_removed = on_peer_removed;
            srv->self3 = srv;
            srv->group3 = udp_group_create(srv->instance, &srv->gp3);
            assert(srv->group3 != NULL);
        }
        /* clients 2..N are added to group 3 */
        err = udp_group_peer_add(srv->group3, peer);
        assert(err == UDP_OK);
    }
}

void on_peer_expired(udp_params_t *params, udp_peer_t *peer, UDPPEER reason) {
    ((server *)params)->num_peers_expired++;
}

void setup_server(server *s) {
    memset(s, 0, sizeof(*s));
    s->params.port = 12345;
    s->params.max_payload_size = 0;
    s->params.app_id = 34;
    s->params.app_version = 3;
    s->params.interface = "127.0.0.1";
    s->params.on_error = on_error;
    s->params.on_idle = on_idle;
    s->params.on_peer_new = on_peer_new;
    s->params.on_peer_expired = on_peer_expired;
    int r = vector_init(&s->packets, sizeof(udp_payload_t *));
    assert(r == 0);
    s->instance = udp_initialize(&s->params);
    assert(s->instance != NULL);
}

void terminate_server(server *s) {
    for (size_t i = 0, n = s->packets.item_count; i != n; ++i) {
        udp_payload_t *pl = *(udp_payload_t **)vector_item_get(&s->packets, i);
        udp_payload_release(pl);
    }
    udp_terminate(s->instance);
    vector_deinit(&s->packets);
    memset(s, 0, sizeof(*s));
}

void c_on_error(udp_client_params_t *cparm, UDPERR err, char const *text) {
    fprintf(stderr, "CLIENT ERROR: %d (%s)\n", err, text);
    client *c = (client *)cparm;
    c->num_errors++;
}

void c_on_idle(udp_client_params_t *cparm) {
    client *c = (client *)cparm;
    c->num_idles++;
}

void c_on_payload(udp_client_params_t *cparm, udp_client_connection_t *conn, udp_payload_t *payload) {
    client *c = (client *)cparm;
    c->num_payloads++;
}

void c_on_disconnect(udp_client_params_t *cparm, udp_client_connection_t *conn, UDPPEER reason) {
    client *c = (client *)cparm;
    c->num_disconnects++;
}

void setup_client(client *c) {
    memset(c, 0, sizeof(*c));
    c->params.on_error = c_on_error;
    c->params.on_idle = c_on_idle;
    int r = vector_init(&c->packets, sizeof(udp_payload_t *));
    assert(r == 0);
    c->client = udp_client_initialize(&c->params);
    assert(c->client != NULL);
}

void terminate_client(client *c) {
    for (size_t i = 0, n = c->packets.item_count; i != n; ++i) {
        udp_payload_t *pl = *(udp_payload_t **)vector_item_get(&c->packets, i);
        udp_payload_release(pl);
    }
    udp_client_terminate(c->client);
    vector_deinit(&c->packets);
    memset(c, 0, sizeof(*c));
}


int main() {
    setup_server(&server1);
    step_server(&server1);

    setup_client(&client1, "ClientA");
    step_client(&client1);
    step_server(&server1);
    step_client(&client1);
    step_server(&server1);
    step_client(&client1);

    setup_client(&client2, "ClientB");
    step_client(&client1);
    step_client(&client2);
    step_server(&server1);
    step_client(&client1);
    step_client(&client2);
    step_server(&server1);
    step_client(&client1);
    step_client(&client2);

    terminate_client(&client1);
    terminate_client(&client2);
    terminate_server(&server1);

    return 0;
}

