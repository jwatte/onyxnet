#if !defined(onyxudp_types_h)
#define onyxudp_types_h

#include <stdint.h>
#include <pthread.h>
#include <onyxutil/hashtable.h>
#include <onyxutil/vector.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct udp_peer_t udp_peer_t;
typedef struct udp_group_t udp_group_t;
typedef struct udp_instance_t udp_instance_t;
typedef struct udp_client_t udp_client_t;
typedef struct udp_client_connection_t udp_client_connection_t;
typedef struct udp_params_t udp_params_t;
typedef struct udp_client_params_t udp_client_params_t;

/* internal types used by the library */

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
    udp_addr_t address;
    uint64_t last_receive_timestamp;
    uint64_t last_send_timestamp;
    /* Used to be able to down-version communications with the peer */
    uint16_t remote_app_version;
    udp_instance_t *instance;
    vector_t out_queue;
    vector_t groups;
};

enum UDPCONNECTIONSTATE {
    UDPCNS_PRECONNECT,
    UDPCNS_INITIAL,
    UDPCNS_CONNECTED,
    UDPCNS_FINAL,
    UDPCNS_DEAD
};

struct udp_client_t {
    udp_client_params_t *params;
    int socket;
    int running;
    UDPCONNECTIONSTATE state;
    pthread_t thread;
    hash_table_t connections;
};

struct udp_client_connection_t {
    udp_conn_addr_t addr;
    udp_client_t *client;
    udp_payload_t *conn_payload;
    vector_t outgoing;
    uint64_t last_transmit;
    uint64_t last_receive;
    size_t ntransmit;
    UDPCONNECTIONSTATE state;
};

struct udp_payload_owner_t {
    udp_params_t            *server;
    udp_client_params_t     *client;
};

#if defined(__cplusplus)
}
#endif

#endif  //  onyxudp_types_h
