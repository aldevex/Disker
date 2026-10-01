#pragma once
#include "./base.h"

// Get endianness
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    #define UTILS_LE (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#else
    #error "failed to get endiannes because your compiler lacks de facto standard C macros"
#endif



// Core swap macros
#if defined(__GNUC__) || defined(__clang__)
    #define _utilsSwap16(x) __builtin_bswap16(x)
    #define _utilsSwap32(x) __builtin_bswap32(x)
    #define _utilsSwap64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
    #define _utilsSwap16(x) _byteswap_ushort(x)
    #define _utilsSwap32(x) _byteswap_ulong(x)
    #define _utilsSwap64(x) _byteswap_uint64(x)
#else
    #define _utilsSwap16(x) (\
        (x << 8)\
        | (x >> 8)\
    )
    #define _utilsSwap32(x) (\
        ((x << 24) & 0xFF000000)\
        | ((x << 8) & 0x00FF0000)\
        | ((x >> 8) & 0x0000FF00)\
        | ((x >> 24) & 0x000000FF)\
    )
    #define _utilsSwap64(x) (\
        ((x << 56) & 0xFF00000000000000ULL)\
        | ((x << 40) & 0x00FF000000000000ULL)\
        | ((x << 24) & 0x0000FF0000000000ULL)\
        | ((x << 8)  & 0x000000FF00000000ULL)\
        | ((x >> 8)  & 0x00000000FF000000ULL)\
        | ((x >> 24) & 0x0000000000FF0000ULL)\
        | ((x >> 40) & 0x000000000000FF00ULL)\
        | ((x >> 56) & 0x00000000000000FFULL)\
    )
#endif
// Generic swap
#define _utilsSwapG(x) _Generic((x),\
    uint16_t: _utilsSwap16(x),\
    uint32_t: _utilsSwap32(x),\
    uint64_t: _utilsSwap64(x)\
)(x)



#if UTILS_LE
    // Native endian to LE
    #define utils_nvToLE(x) x
    // LE to native endian
    #define utils_nvFromLE(x) x

    // Native endian to BE
    #define utils_nvToBE(x) _utilsSwapG(x)
    // BE to native endian
    #define utils_nvFromBE(x) _utilsSwapG(x)
#else
    // Native endian to BE
    #define utils_nvToBE(x) x
    // BE to native endian
    #define utils_nvFromBE(x) x

    // Native endian to LE
    #define utils_nvToLE(x) _utilsSwapG(x)
    // LE to native endian
    #define utils_nvFromLE(x) _utilsSwapG(x)
#endif
