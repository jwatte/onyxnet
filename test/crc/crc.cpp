#include <onyxutil/crc.h>

#include <assert.h>
#include <stdio.h>

struct test {
    char const *data;
    size_t dsize;
    uint32_t crc32;
    uint16_t crc16;
};

test tests[] = {
    { "", 0, 0x0, 0x0 },
    { "", 1, 0xd202ef8d, 0xed84 },
    { "covfefe", 7, 0xf62cd904, 0xe1a7 },
    { "\0\0\0\0\0\0\0\0", 8, 0x6522df69, 0xf4ae },
    { "abcdefghijklmnopqrstuvwxyz", 26, 0x4c2750bd, 0xe4a5 },
    { "The quick brown fox jumps over the lazy dog.", 44, 0x519025e9, 0xfa25 },
    { NULL }
};

int main() {
    for (int i = 0; tests[i].data; ++i) {
        uint32_t crc32 = update_crc32(tests[i].data, tests[i].dsize, 0);
        uint16_t crc16 = update_crc16(tests[i].data, tests[i].dsize, 0);
        assert(crc32 == tests[i].crc32);
        assert(crc16 == tests[i].crc16);
        crc32 = 0;
        for (size_t q = 0; q != tests[i].dsize; ++q) {
            crc32 = update_crc32(tests[i].data+q, 1, crc32);
        }
        assert(crc32 == tests[i].crc32);
        crc16 = 0;
        for (size_t q = 0; q != tests[i].dsize; ++q) {
            crc16 = update_crc16(tests[i].data+q, 1, crc16);
        }
        assert(crc16 == tests[i].crc16);
    }
    return 0;
}

