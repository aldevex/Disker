// Base codebase-wide used and platform-specific (C standard and/or OS API) definitions
#pragma once



// Compiler-specific macros

#if defined(__clang__) || defined(__GNUC__)
    #define ATTRIB_PACKED __attribute__((packed))
    #define ATTRIB_UNUSED __attribute__((unused))
#else
    #error your compiler is not registered in the compiler-specific code,\
            and may be unable to provide expected definitions.
#endif

#define ATTRIB_NORETURN _Noreturn



// Global base types

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef unsigned char byte_t;
typedef char UTF8_t;
typedef uint16_t UTF16_t;

// Three-state boolean
typedef byte_t tribool_t;
#define TRIBOOL_FALSE 0
#define TRIBOOL_TRUE  1
#define TRIBOOL_NONE  2

// Error State
typedef bool es_t;
#define ES_FAILURE false
#define ES_SUCCESS true

typedef void* handle_t;
#ifdef _WIN32
    #define HANDLE_NONE ((handle_t)-1)
#endif



// Helper macros

// "Free, return" (sets result and jumps to "end" label)
#define fret(x) do {\
    result = x;\
    goto end;\
} while (0)

// "Free, return" (jumps to "end" label)
#define fretvoid do {\
    goto end;\
} while (0)

// Length of array
#define lenof(x) ( sizeof(x)/sizeof(*x) )

// Endianness
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    #define BASE_LE (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#else
    #error "failed to get endiannes because your compiler lacks de facto standard C macros"
#endif

// Internal endian swap macros
#if defined(__GNUC__) || defined(__clang__)
    #define _base_swap16(x) __builtin_bswap16(x)
    #define _base_swap32(x) __builtin_bswap32(x)
    #define _base_swap64(x) __builtin_bswap64(x)
#else
    #define _base_swap16(x) (\
        (x << 8)\
        | (x >> 8)\
    )
    #define _base_swap32(x) (\
        ((x << 24) & 0xFF000000)\
        | ((x << 8) & 0x00FF0000)\
        | ((x >> 8) & 0x0000FF00)\
        | ((x >> 24) & 0x000000FF)\
    )
    #define _base_swap64(x) (\
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

// Swap-X: Swaps byte endianness for buffers of any size
static inline void base_swapX(void* pData, size_t size)
{
    byte_t* bytes = (byte_t*)pData;
    for (size_t first = 0, last = size -1; first < last; first++, last--)
    {
        byte_t firstCopy = bytes[first];
        bytes[first] = bytes[last];
        bytes[last] = firstCopy;
    }
}

// Swap-Auto: Swaps byte endianness from 8 up to 64 bits
#define base_swapA(x) _Generic((x),\
    uint8_t: x,\
    uint16_t: _base_swap16(x),\
    uint32_t: _base_swap32(x),\
    uint64_t: _base_swap64(x)\
)(x)

// White
#define COLOUR_NORMAL "\e[0;37m"
// Bold red
#define COLOUR_ERROR "\e[1;31m"
// Bold yellow
#define COLOUR_WARNING "\e[1;33m"
// Clear screen
#define COLOUR_CLS "\e[H\033[J"



// Core entry point (called by the actual entry point defined in platform-specific implementation)
extern es_t entry(size_t argCount, const UTF8_t *const * args);



// Platform-specific functions

// In a common OS environment, uses the C standard exit()
ATTRIB_NORETURN void base_halt(es_t success);

// If size == 0:
//   If ptr != NULL: Frees given pointer
//   If ptr == NULL: Does nothing
// If size != 0:
//   If ptr == NULL: Returns newly created buffer of given size
//   If ptr != NULL: Returns reallocated buffer of given size
void* base_realloc(void* ptr, size_t size);
void base_memcpy(void* dst, const void* src, size_t size);
void base_memmove(void* dst, const void* src, size_t size);

// Prints given C format with values to the standard output stream
void _base_stdout(const UTF8_t* format, ...);
// Prints given C format with values to the standard error stream
void _base_stderr(const UTF8_t* format, ...);
// Receives segments of a line from the standard input stream
// The end of the received line is signaled by the written buffer ending with "\n\0"
es_t base_input(UTF8_t* pBlock, size_t maxWithNT, size_t* pWrittenWithNT);

#define base_printf(format, ...)\
    _base_stdout(format, __VA_ARGS__)

#define base_warnf(format, ...)\
    _base_stdout(COLOUR_WARNING "Warning: " COLOUR_NORMAL format, __VA_ARGS__)

#define base_stdErrorf(format, ...)\
    _base_stderr(COLOUR_ERROR "Error: " COLOUR_NORMAL format, __VA_ARGS__)

#define base_fatalErrorf(format, ...)\
    _base_stderr(COLOUR_ERROR "Fatal error: " COLOUR_NORMAL format, __VA_ARGS__)

// Available formats: N for decimal, 0bN for binary, 0xN for hexadecimal, 0cN for octal
uint64_t base_strToU64(const UTF8_t* pTextNT, UTF8_t** pPtrAfterEnd);
void base_sscanf(const UTF8_t* source, const UTF8_t* format, ...);
void base_snprintf(UTF8_t* destination, size_t size, const UTF8_t* format, ...);
