#if !defined(onyxbase_crc_h)
#define onyxbase_crc_h

#include <stdint.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

    uint32_t update_crc32(void const *data, size_t size, uint32_t in);
    uint16_t update_crc16(void const *data, size_t size, uint16_t in);

#if defined(__cplusplus)
}
#endif

#endif  // onyxbase_crc_h

