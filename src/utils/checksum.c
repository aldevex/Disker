#include "./checksum.h"

uint32_t utils_calcGPTCRC32(const void* data, size_t length)
{
    static uint32_t table[256];
    static bool initialized = false;

    // Build standard IEEE 802.3 table once
    if (!initialized)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            uint32_t c = i;
            // 0xEDB88320 IEEE 802.3 polynomial
            for (int k = 0; k < 8; k++)
                c = (c & 1) ? (0xEDB88320 ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        initialized = true;
    }

    const uint8_t *p = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++)
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);

    return crc ^ 0xFFFFFFFF; // Final bit inversion
}
