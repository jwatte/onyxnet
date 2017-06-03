#if !defined(onyxudp_udpclient_h)
#define onyxudp_udpclient_h

#include "udpbase.h"

#if !defined(__cplusplus)
extern "C" {
#endif

    typedef struct udp_client_t udp_client_t;
    typedef struct udp_client_connection_t udp_client_connection_t;
    
    /* Specify behavior for the allocated UDP client.
     * If you need more information passed along with the callbacks, you can put the 
     * udp_client_params_t struct as the first member of a bigger struct, and put 
     * your own information after the udp_client_params_t, and use casting to convert
     * from pointer to one to pointer to the other.
     */
    typedef struct udp_client_params_t {
        /* app_id must match that of the remote server.
         */
        uint16_t            app_id;

        /* app_version must be less than or equal to app_version for the remote server.
         */
        uint16_t            app_version;

        /* The size of the biggest payload that can be allocated and sent. The UDP library 
         * as well as underlying network layers will add to this size, so to stick under 
         * the largest non-fragmented IPv6 packet size, make this 1200 or less.
         * If you make it 0, it will be filled in with the appropriate default size to 
         * work well on IPv4 and IPv6 networks.
         */
        uint16_t            max_payload_size;

        /* The error callback is called when some error happens inside the library. The error may 
         * or may not be recoverable. Errors during initialization are also reported here. For 
         * functions that do not report an error code, the error callback is the best way to find 
         * out what, exactly, went wrong.
         * @param params The instance that had the error.
         * @param code A code specifying the specific error (@see UDPERR codes below.)
         * @param text A short text describing the error. This will contain programmer-speak, and 
         * is not necessarily suitable for displaying to end users in GUIs, but may be helpful 
         * in log files.
         * @note This will be called from whatever thread is calling into the UDP library, which 
         * will be the UDP library thread if you use udp_run_client() or your own thread otherwise.
         */
        void                (*on_error)(udp_client_params_t *params, UDPERR code, char const *text);

        /* When using the udp_run_client() threaded implementation, the idle() function will be called 
         * with some frequency, to give your application a chance to do work that has to be done 
         * from within the polling thread.
         * @param params The instance that is idling.
         * @note This will only be called if you use udp_client_run(), not if you use udp_client_poll().
         */
        void                (*on_idle)(udp_client_params_t *params);

        /* When receiving some payload, here it's delivered to the application. The application borrows 
         * a refcount to the payload within this callback. If it wants to hang on to the payload 
         * (rather than copy the data out in the callback) it needs to call udp_payload_hold(), and 
         * later balance that with a call to udp_payload_release() from within the same thread.
         * @param params The client that received the payload.
         * @param conn The connection that the payload was received on.
         * @param payload The payload that was received.
         */
        void                (*on_payload)(udp_client_params_t *params, udp_client_connection_t *conn, udp_payload_t *payload);

        /* When a connection has been idle for too long, or sometimes because of detectable network errors, 
         * a connection will be detected as "down" and removed. Here is how the application is told about 
         * this. This will also be called when manually diconnecting.
         * @param params The client that had the connection open.
         * @param conn The connection that lapsed.
         * @param reason Why the connection lapsed.
         */
        void                (*on_disconnect)(udp_client_params_t *params, udp_client_connection_t *conn, int reason);
    } udp_client_params_t;
    
    /* Allocate a UDP client. This opens a socket, which can be used to connect to zero or more 
     * remote hosts, using @see udp_client_connect().
     * @param params The parameters for the client allocated. This is not copied; the 
     * params pointed to by this value must stay valid for as long as this client is 
     * alive.
     * @return An allocated client structure. You only need one of these in your application for 
     * most cases, as the same structure can keep more than one connection alive.
     */
    udp_client_t *udp_client_initialize(udp_client_params_t *params);

    /* Shut down the client thread if it's running with udp_client_run().
     * Dispose any memory used by the client structure. This will not send disconnect 
     * messages to established connections; you have to do that yourself if you care.
     * @param client The prviously @see udp_client_initialize() initialized client.
     */
    void udp_client_terminate(udp_client_t *client);

    /* This function resolves a text-format address into a binary-format address suitable 
     * for using in udp_client_connect(). Calling this function may block the calling thread 
     * for some time, depending on DNS availability. You may want to call this on a thread 
     * of your choosing.
     * @param in_addr A text format address to resolve.
     * @param out_addr A resolved, binary format address, or nothing.
     * @return 0 on success, or an error on failure
     * @note This can be called from any thread, but will block that thread.
     */
    UDPERR udp_client_address_resolve(udp_addr_t *in_addr, udp_conn_addr_t *out_addr);

    /* Given an address, start connecting to that address. You won't know whether 
     * connected or not until you receive either a payload back, or a disconnect message.
     * @param client The client that is connecting.
     * @param addr The address to connect to.
     * @param payload The first packet to send. This packet may be repeatedly sent until the server responds.
     * Can be NULL, in which case a logically empty packet is sent (and returned.) This function takes 
     * ownership of the argument, even if it returns an error.
     * @return A udp_client_connection_t structure, or NULL for error (such as duplicate address to 
     * another existing connection.) The actual error is reported through on_error().
     */
    udp_client_connection_t *udp_client_connect(udp_client_t *client, udp_conn_addr_t *addr, udp_payload_t *payload);

    /* Disconnect a client connection, or stop trying to connect if not yet connected.
     * The on_disconnect() callback will be called for this connection.
     * @param client The connection to disconnect.
     * @return 0 for success, or a UDPERR error.
     */
    UDPERR udp_client_disconnect(udp_client_connection_t *conn);
    
    /* Send a packet to the given connection. Once the packet is sent, it is destroyed, unless you have 
     * called udp_payload_hold() on it before passing it in here.
     * @param conn the connection to send to
     * @param payload the payload to send
     * @note UDP packet sending is UDP-best-effort only, which may or may not send the packet, and may 
     * or may not re-order the packet, and may or may not duplicate the packet before it's received. 
     * See the higher-layer libraries for how to put various other guarantees on top of this simple 
     * primitive.
     */
    UDPERR udp_client_payload_send(udp_client_connection_t *conn, udp_payload_t *payload);

    /* Run client service in a thread of its own. Callbacks to your application will be made on that 
     * thread. The only function valid to call from your main thread in this case is udp_client_terminate().
     * All other calls should be done in response to callbacks (of which on_idle() may be convenient.)
     * @param client The client to run service for.
     * @return 0 on success, else an error
     * @note You either use udp_client_poll() or @see udp_client_run(), not both. Only call this function 
     * once per given client.
     */
    UDPERR udp_client_run(udp_client_t *client);

    /* Poll a UDP client, sending and receiving what's needed for this point in time. This will call 
     * callbacks back into your application, so make sure you're prepared for that. You can call this 
     * function once through each iteration of your main loop if you don't want to use a separate 
     * thread for networking.
     * @param client The client to poll.
     * @return 0 if nothing happened; > 0 if payloads were processed or other things happened.
     * @note You either use udp_client_poll() or @see udp_client_run(), not both.
     */
    int udp_client_poll(udp_client_t *client);

    /* @see udp_payload_get()
     * @param client The client context to allocate a payload from.
     * @return The allocated payload, or NULL for failure.
     * @note Payloads are NOT fungible between instances / clients. If allocated on a particular client, 
     * only use it with instances of that client function, and on that client's thread.
     */
    udp_payload_t *udp_client_payload_get(udp_client_t *instance);

#if !defined(__cplusplus)
}
#endif

#endif  //  onyxudp_udpclient_h

