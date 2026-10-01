#pragma once
#include "./darray.h"
#include "./string.h"

// Creates compound literal like view8MakeCopyNT
#define vw(nt) ((View8)\
    { ._pBuffer = nt, ._count = countNTSize8(nt), ._terminated = true}\
)
// Creates compound literal like view8MakeCopyS
#define vwstr(pStr) ((View8)\
    { ._pBuffer = (pStr)->_pBuffer, ._count = (pStr)->_count, ._terminated = true}\
)
// Printf spread
#define pfSpread(pTextualObject)\
    (int)((pTextualObject)->_count), (pTextualObject)->_pBuffer

// Free, return
#define fret(x) do {\
    result = x;\
    goto end;\
} while (0)
// Free, return
#define fretvoid do {\
    goto end;\
} while (0)

// Length of array
#define lenof(x) ( sizeof(x)/sizeof(x[0]) )
// Get typeof extension if it exists
#if defined(__GNUC__) || defined(__clang__)
    #define _memTypeof __typeof__
#endif
// Use bounds checking "at" function if typeof exists
#ifdef _memTypeof
    // Object internal buffer indexing
    #define at(pObject, i) (\
        *(_memTypeof((pObject)->_pBuffer))\
        _memAt(&(pObject)->_generic, i, sizeof((pObject)->_pBuffer[0]))\
    )
#else
    // Object internal buffer indexing
    #define at(pObject, i) ( (pObject)->_pBuffer[(i)] )
#endif
