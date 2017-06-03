#if !defined(onyxudp_protocol_h)
#define onyxudp_protocol_h

/* Packet format:
 *
 * All data fields are little-endian. Sorry, network gurus, but the world is 
 * little endian, and doing the work to swap the data in both sending and 
 * receiving ends (both of which run x86 or ARM in little-endian mode) makes 
 * no sense.
 *
 * <8-byte packet:
 * invalid
 *
 * 8-byte control packet:
 * crc16 (2 bytes)
 * command (2 bytes)
 * app_id (2 bytes)
 * app_version (2 bytes)
 *
 * >8-byte data packet:
 * crc32 (4 bytes)
 * app_id (2 bytes)
 * app_version (2 bytes)
 * <data> <N bytes>
 *
 * In each case, the CRC is calculated on all data following the CRC field 
 * to the end of the packet.
 *
 * For later versions of the protocol, perhaps cryptography will be added, in which 
 * case more fields will go into the header.
 */

/* 8-byte packets have a crc16, and a command and application identifying information.
 */
struct command_header {
    uint16_t crc16;             //  command, app_id, app_version
    uint16_t command;
    uint16_t app_id;
    uint16_t app_version;
};

/* Data packets have a 32-bit CRC of the data and application ids, as well as the 
 * application ids, followed by the data.
 */
struct data_header {
    uint32_t crc32;             //  app_id, app_version, <data>
    uint16_t app_id;
    uint16_t app_version;
};

enum {
    UDP_CMD_CONNECT = 1,
    UDP_CMD_DISCONNECT = 2
};

#endif  //  onyxudp_protocol_h
