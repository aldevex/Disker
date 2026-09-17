#pragma once
#include "./darray.h"
#include "./string.h"

// Creates compound literal like view8MakeCopyNT
#define vw(nt) ((View8)\
    { ._pBuffer = nt, ._itemsCount = countNTSize8(nt), ._terminated = true}\
)
// Creates compound literal like view8MakeCopyS
#define vwstr(pStr) ((View8)\
    { ._pBuffer = (pStr)->_pBuffer, ._itemsCount = (pStr)->_itemsCount, ._terminated = true}\
)

// Free, return
#define fret(x) do {\
    result = x;\
    goto end;\
} while (0)
// Free, return
#define fretvoid do {\
    goto end;\
} while (0)

// Object internal buffer indexing
#define at(pObject, i) ( (pObject)->_pBuffer[(i)] )
// Printf spread
#define pfSpread(pTextualObject)\
    (int)((pTextualObject)->_itemsCount), (pTextualObject)->_pBuffer
