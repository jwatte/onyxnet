#if !defined(udp_udpbase_h)
#define udp_udpbase_h

/* @page INTRODUCTION
 *
 * This library provides a simple-to-use communications layer on top of UDP. The core 
 * functions are @see udp_initialize() for a server, and @see udp_client_initialize() 
 * for a client. These functions are independent, and you can allocate zero or more of 
 * each of those structures in your program.
 *
 * You can poll each of the structures yourself using @see udp_poll() and @see udp_client_poll(), 
 * or you can start threads that take care of this using @see udp_run() and @see udp_client_run().
 * Note that, if you use threads, the callbacks into your program will be called within the 
 * context of those threads. The UDP library does not do any thread synchronization of its own, 
 * and is designed so that if you follow some simple threading rules, you only need to 
 * synchronize your own application.
 */

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

    /* Represents a peer; another process running typically on some other machine, and sending
     * packets to us.
     */
    typedef struct udp_peer_t udp_peer_t;
    /* Represents a logical "group" of peers that has been created by the app. */
    typedef struct udp_group_t udp_group_t;
    /* Represents the actual listening socket and library support for that. */
    typedef struct udp_instance_t udp_instance_t;

    /* Possible errors returned by the UDP library. */
    enum UDPERR {
        /* Everything's hunky-dorey in the world. */
        UDP_OK = 0,
        /* This is an error where an allocation fails. */
        UDPERR_OUT_OF_MEMORY = 1,
        /* This is an error with the networking/socket system, such as connection broken. */
        UDPERR_SOCKET_ERROR = 2,
        /* This is an error in the underlying I/O subsystem. */
        UDPERR_IO_ERROR = 3,
        /* This is an error related to the address -- such as host not found. */
        UDPERR_ADDRESS_ERROR = 4,
        /* This is an error you could have prevented by passing in valid arguments. */
        UDPERR_INVALID_ARGUMENT = 5
    };

    /* Possible peer reasons */
    enum UDPPEER {
        /* No traffic for some amount of time makes Jack a dull boy. */
        UDPPEER_TIMEDOUT = 1,
        /* Peer connections are dropped when the last group of the peer is destroyed. */
        UDPPEER_LAST_GROUP_DESTROYED = 2,
        /* Peers may be disconnected explicitly through function calls. Note that if the 
         * disconnect message is lost, the peer may instead get to UDPPEER_TIMEDOUT state.
         */
        UDPPEER_CLIENT_DISCONNECTED = 3
    };

    /* Payloads are the data within UDP packets (outside of framing/addressing information.)
     * This is what you "send" and "receive." The ownership and structure of this data is 
     * internal to the UDP library, but the structure is exposed so that you can efficiently 
     * talk about data without having to indirect through accessor functions. You may not 
     * allocate this structure yourself and hand it to the library as if it were a legitimate 
     * payload. You may not change the values of the structure. You may not attempt to 
     * interpret the _donttouch members.
     */
    typedef struct udp_payload_t {
        /* The actual payload data -- don't change this pointer */
        void                *data;
        /* How much payload data there is */
        uint16_t            size;
        /* Don't touch this field */
        uint16_t            _refcount;
        /* The app_id that sent this payload (must match yours to be received) (used on server only) */
        uint16_t            app_id;
        /* The app_version that sent this payload (must be <= yours to be received) (used on server only) */
        uint16_t            app_version;
        /* Don't touch this field */
        void                *_owner;
    } udp_payload_t;

    /* Parameters for the instantiation of the UDP library.
     * This defines how your application will use the library.
     * The pointer to this structure that you pass to udp_initialize() must be valid for the 
     * lifetime of the library instance, because a copy is not made.  If you
     * need additional information, you may create an aggregate struct that contains udp_params_t 
     * as a member, and pass a pointer to the member; then recover the outer struct using casting.
     *
     *      struct my_data {
     *          udp_params_t        params;     // first
     *          void                *my_data;
     *      };
     *
     *      my_data md = { ... };
     *      udp_initialize(&md.params);
     *
     *      void on_peer_new(udp_params_t *params, ...) {
     *          my_data *md = (my_data *)params;
     *          ...
     *      }
     */
    typedef struct udp_params_t {
        /* What UDP port to listen for clients on. Should be in the range 1..65535, and typically
         * in the range 2049..8191 for best compatibilty with various system services. If the 
         * value is 0, the default of 4711 is used.
         */
        uint16_t            port;

        /* The maximum possible payload size. Should be in the range of 4..65496, and generally 
         * should be in the low thousands. If the value is 0, the default of 1200 is used, which 
         * is a value that works over almost all networks. (larger values may have trouble with 
         * IPv6 fragmentation, depending on factors.)
         */
        uint16_t            max_payload_size;

        /* An ID that is unique to your application, to tell your application apart from other 
         * applications that may also be using the same UDP port on the same network at the same 
         * time. Pick a random number. No, more random. NO, MORE RANDOM!
         */
        uint16_t            app_id;

        /* The version of your application. Each outgoing packet is tagged with the version that 
         * sent it, and the receiving application must be of the same version or higher to not 
         * filter it out, unless a message has already been sent to the address from which the 
         * higher version was received.
         */
        uint16_t            app_version;

        /* If interface is not NULL, then it needs to specify the address of a local network 
         * interface on which to listen for connections. When it is NULL, the UDP library will 
         * listen to all the machine interfaces. To only listen to the internal loopback interface 
         * you might specify the string "127.0.0.1".
         */
        char const          *interface;

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
         * will be the UDP library thread if you use udp_run() or your own thread otherwise.
         */
        void                (*on_error)(udp_params_t *params, UDPERR code, char const *text);

        /* When using the udp_run() threaded implementation, the idle() function will be called 
         * with some frequency, to give your application a chance to do work that has to be done 
         * from within the polling thread.
         * @param params The instance that is idling.
         * @note This will only be called if you use udp_run(), not if you use udp_poll().
         */
        void                (*on_idle)(udp_params_t *params);

        /* When a packet is received from an IP address/port that doesn't currently have an active 
         * peer attached to a group, the library will forward the payload to this callback.
         * If you want to refer to the payload after this callback returns, you must call 
         * udp_payload_hold() on it, and later call udp_payload_release() when done. You borrow a 
         * reference from the library while inside this callback.
         * @param params The instance that is idling.
         * @param peer The new (temporary?) peer that sent the packet.
         * @param payload The packet sent from the peer.
         * @note If you want to keep receiving packets from this peer, and be able to send data back 
         * to the peer, you must add the peer to a group from within this callback.
         */
        void                (*on_peer_new)(udp_params_t *params, udp_peer_t *peer, udp_payload_t *payload);

        /* When a peer is removed from all groups, it will "expire" and no longer be recognized by 
         * the library. If the remote computer sends another packet, you will get another on_new_peer 
         * callback. To tell you that the peer expired, this callback is called.
         * A peer can also "expire" and be implicitly removed from all groups by not sending data to 
         * the server for a certain amount of time (a timeout.)
         * @param params The instance that is removing the peer.
         * @param peer The peer being removed.
         * @param reason The reason code (timeout or removed)
         */
        void                (*on_peer_expired)(udp_params_t *params, udp_peer_t *peer, UDPPEER reason);
    } udp_params_t;

    /* You pass in udp_group_params_t to a call to udp_group_create(). The pointer to this struct 
     * must be valid until that group is deleted, because a copy is not made by the library. If you
     * need additional information, you may create an aggregate struct that contains udp_group_params_t 
     * as a member, and pass a pointer to the member; then recover the outer struct using casting.
     *
     *      struct my_group {
     *          udp_group_params_t  params;     // first
     *          void                *my_data;
     *      };
     *
     *      my_group mg = { ... };
     *      udp_group_create(&mg.params);
     *
     *      void on_peer_message(udp_group_params_t *params, ...) {
     *          my_group *mg = (my_group *)params;
     *          ...
     *      }
     */
    typedef struct udp_group_params_t {
        /* When payloads are received for peers in in this group, those payloads are routed to this callback.
         * @param params Your group parameters for this group.
         * @param peer The peer that received this data.
         * @param payload The data that was received.
         * @note The payload is forwarded to potentially multiple groups, so if you use udp_payload_hold() 
         * beware that the pointer is likely shared between groups. If you are done with the payload data 
         * when this callback returns, you do not need to call udp_payload_hold() or udp_payload_release(); 
         * you are borrowing the refcount while inside this callback.
         */
        void                (*on_peer_message)(udp_group_params_t *params, udp_peer_t *peer, udp_payload_t *payload);
        /* When a peer times out, or is somehow otherwise kicked out of a group, this 
         * callback is called.
         * @param params Your group parameters from this group.
         * @param peer The peer that left the group.
         * @param reason Why the peer is being removed.
         * @note This function may be called from the library servicing the peer detecting a timeout, 
         * or from a call to udp_group_peer_remove().
         */
        void                (*on_peer_removed)(udp_group_params_t *params, udp_peer_t *peer, int reason);
    } udp_group_params_t;

    /* Represent an internet address in text. This will typically be stored as a dotted-quad 
     * for IPv4, or colon-hex format for IPv6. The port will be decimal digits.
     * @see udp_peer_address_format()
     */
    typedef struct udp_addr_t {
        /* Dotted-quad / colon-hex format IP address */
        char                addr[120];
        /* Decomal port number */
        char                port[8];
    } udp_addr_t;

    /* A binary address with opaque internal format.
     */
    typedef struct udp_conn_addr_t {
        unsigned char data[32];
    } udp_conn_addr_t;

    /* Create a new listening UDP socket, and context. You have to udp_run() it before 
     * it actually does any work! Or you can call udp_poll() on it yourself from a thread
     * of your choosing.
     * @param params Describes the context you want to create, and provides callbacks for 
     * the UDP library to inform your application about events. This pointer must stay 
     * valid for the lifetime of the instance.
     */
    udp_instance_t *udp_initialize(udp_params_t *params);

    /* If you want the UDP library to run the network on its own thread, call udp_run() 
     * after you udp_initialize(). The UDP library will then call your callbacks in some 
     * thread that is not your main thread!
     * @param instance the instance you previously initialized with udp_initialize()
     * @return 0 for success, or an error code.
     * @note Call this only once for a given instance. You use either udp_run() or @see 
     * udp_poll() for a given instance, not both.
     */
    UDPERR udp_run(udp_instance_t *instance);

    /* If you want to control which thread the UDP library runs in, including your own main 
     * thread, you can call udp_poll() each time through your main loop instead of calling 
     * udp_run() once.
     * @param instance the instance you previously initialized with udp_initialize()
     * @return some positive number for number of packets/messages processed, 0 for nothing 
     * to do (or error.) Errors are reported through the error callback in your params.
     * @note call this from one thread only, and make sure it has returned before you call 
     * udp_terminate(). You use either udp_poll() or @see udp_run() in your program, not 
     * both.
     */
    int udp_poll(udp_instance_t *instance);

    /* Close the socket, free memory, and stop any running thread, not necessarily in that 
     * order. If you want to make sure no leaks are reported out of the UDP library when 
     * your application quits, you must call this function before quitting, once for each 
     * instance you have previously initialized.
     * @param instance A previously obtained instance from udp_initialize()
     * @note call this from the main thread, and not while you have some thread inside 
     * udp_poll().
     */
    void udp_terminate(udp_instance_t *instance);

    /* Groups are how the UDP library knows which peers (remote users) are allowed to talk 
     * to your application. Inside the on_new_peer() callback, you should attach a peer to 
     * a group if you want to keep receiving data from that peer. You can use a single 
     * group for all peers, or some number of groups for whatever organization you need. A 
     * peer can belong to more than one group, and incoming data will then be forwarded to 
     * each group.
     * @param instance the allocated instance you want to create the group for
     * @param params defines specifics of the group you want to create. This pointer must 
     * stay valid for the lifetime of the group.
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    udp_group_t *udp_group_create(udp_instance_t *instance, udp_group_params_t *params);

    /* Destroy a group, and release any peers attached to that group. Peers who are not 
     * attached to any other group will then expire.
     * @param group The group to release.
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    void udp_group_destroy(udp_group_t *group);

    /* Add a peer to a group. This is how you hang on to peers after you have received their 
     * initial connection message. A peer can belong to more than one group. As long as a 
     * peer belongs to at least one group, the peer will not expire.
     * @param peer the peer to attach to the group
     * @group the group to which to attach the peer
     * @return 0 on success, else an error code
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    UDPERR udp_group_peer_add(udp_peer_t *peer, udp_group_t *group);

    /* Remove a peer from a group to which they belong. Once a peer has been removed from 
     * all groups, the peer will expire.
     * @param peer The peer to remove from the group.
     * @param group The group from which to remove the peer.
     * @return 0 on success, else an error code
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    UDPERR udp_group_peer_remove(udp_peer_t* peer, udp_group_t *group);

    /* Get a list of all peers within a group.
     * @param opeers a pointer to an array to fill in with peer pointers.
     * @param nmax the size of the output array, in number of pointes.
     * @param group the group to get peers from.
     * @return the number of peers returned (which may be 0) OR -1 for an error, 
     * typically if the array is too small.
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    int udp_group_peers_peek(udp_peer_t **opeers, int nmax, udp_group_t *group);

    /* Get a list of all groups that a peer belongs to.
     * @param ogroups a pointer to an array to fill in with group pointers.
     * @param nmax the size of the output array, in number of pointes.
     * @param peer the peer to get groups from.
     * @return the number of groups returned (which may be 0) OR -1 for an error, 
     * typically if the array is too small.
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    int udp_peer_groups_peek(udp_group_t **ogroups, int nmax, udp_peer_t *peer);

    /* Given a payload, enqueue it for sending to every peer within the given group.
     * @param group The group to broadcast the payload to.
     * @param payload The payload to send, previously received from udp_payload_get(). 
     * This call will take ownership of the refcount of the payload, you should not
     * call udp_payload_release() on it.
     * @return 0 for success, else an error code
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    UDPERR udp_group_payload_enqueue(udp_group_t *group, udp_payload_t *payload);

    /* Given a payload, enqueue it for sending to one peer.
     * @param peer The peer to send the payload data to.
     * @param payload The payload to send, previously received from udp_payload_get().
     * This call will take ownership of the refcount of the payload, you should not
     * call udp_payload_release() on it.
     * @return 0 for success, else an error code
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    UDPERR udp_peer_payload_enqueue(udp_payload_t *payload, udp_peer_t *peer);

    /* Get or make an empty payload object that you can put data into.
     * @param instance The context within which to get the payload. The payload can be 
     * sent only to peers/groups that belong to that instance.
     * @return The allocated empty payload, or NULL for error.
     * @note call this from the same thread that calls udp_poll() if you use udp_poll(), or
     * from within a callback from the UDP library if you use udp_run()
     */
    udp_payload_t *udp_payload_get(udp_instance_t *instance);

    /* Given a payload, release its refcount. You may never need to call this, unless you 
     * call udp_payload_get() without then calling udp_payload_enqueue on it, or call 
     * udp_payload_hold() on payloads passed to your callback functions.
     * @param payload The payload to releae.
     * @note Call this in the same thread that you received the payload in.
     */
    void udp_payload_release(udp_payload_t *payload);

    /* Given a payload, hold an extra refcount to it. This is mainly useful if you want to 
     * hang on to payloads you receive from a callback after that callback has returned.
     * The payload object will belong to you until you balance each call to udp_payload_hold() 
     * with a call to udp_payload_release().
     * @param payload The payload to hold on to.
     * @note Call this in the same thread that you received the payload in.
     */
    void udp_payload_hold(udp_payload_t *payload);

    /* Given a UDP peer, format their address in a semi-readable format. Typically, this will 
     * convert an IP address into dotted-quad or colon-hex format, and the port number to 
     * decimal format.
     * @param peer The peer whose address you want to format.
     * @param o_addr A pointer to the address structure where the string-formatted peer address 
     * will be placed.
     * @note Call this from the thread you received the peer in.
     */
    void udp_peer_address_format(udp_peer_t *peer, udp_addr_t *o_addr);

    /* Access to the internal UDP system clock.
     * @return a timestamp in microseconds.
     * @note This timestamp starts at 0 when udp_initialize() is called.
     */
    uint64_t udp_timestamp();

    enum {
        UDP_DEFAULT_MAX_PAYLOAD_SIZE = 1200,
        UDP_MIN_PAYLOAD_SIZE = 32
    };

#if defined(__cplusplus)
}
#endif

#endif  //  udpbase_h

