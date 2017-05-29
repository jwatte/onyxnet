#if !defined(udpbase_h)
#define udpbase_h

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

    typedef struct udp_params_t {
        uint16_t            port;
        void                (*error)(udp_params_t *params, int code, char const *text);
    } udp_params_t;
    typedef struct udp_instance_t udp_instance_t;

    udp_instance_t *initialize_udp(udp_params_t *params);
    void terminate_udp(udp_instance_t *);

    enum {
        NET_OUT_OF_MEMORY = 1,
        NET_SOCKET_ERROR = 2,
        NET_IO_ERROR = 3,
        NET_ADDRESS_ERROR = 4
    };

#if defined(__cplusplus)
}
#endif

#endif  //  udpbase_h

