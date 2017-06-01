#include <onyxudp/udpbase.h>
#include <onyxudp/udpclient.h>
#include <onyxutil/vector.h>

#include <assert.h>
#include <stdio.h>


struct server {
    udp_params_t params;
    udp_instance_t *instance;
    udp_group_t *group1;
    udp_group_t *group2;
    udp_group_t *group3;
    vector_t packets;
};

struct client {
    udp_client_params_t params;
    udp_client_t *client;
    vector_t packets;
};

server server1;
client client1;
client client2;

void setup_server(server *s) {
    memset(s, 0, sizeof(*s));
    s->params.port = 12345;
    s->params.max_payload_size = 0;
    s->params.app_id = 34;
    s->params.app_version = 3;
    s->interface = "127.0.0.1";
    s->on_error = on_error;
    s->on_idle = on_idle;
    s->on_peer_new = on_peer_new;
    s->on_peer_expired = on_peer_expired;
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

void setup_client(client *c) {
    memset(c, 0, sizeof(*c));
    c->on_error = c_on_error;
    c->on_idle = c_on_idle;
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

