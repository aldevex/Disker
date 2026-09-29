#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "./core.h"

#define DARRAY_DEF(structName, fPrefix, Type)\
\
static const Type _darray##structName##Null = {0};\
\
typedef union structName\
{\
    _memGS _generic;\
    struct {\
        Type* _pBuffer;\
        size_t _count;\
        size_t _capacity;\
    };\
} structName;\
\
/* Creation, destruction, and generic allocation and size changes */\
static inline _mem_FUNC_ATTRIB void fPrefix##FillV(structName* pDarray, size_t count, const Type vItem)\
{\
    _memFill(&pDarray->_generic, count, &vItem, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##FillR(structName* pDarray, size_t count, const Type* pItem)\
{\
    _memFill(&pDarray->_generic, count, pItem, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##CopyD(structName* pDarray, const structName* pSrcDarray)\
{\
    _memCopy(&pDarray->_generic, pSrcDarray->_pBuffer, sizeof(Type), pSrcDarray->_count);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##CopyPS(structName* pDarray, const Type* pItems, size_t itemCount)\
{\
    _memCopy(&pDarray->_generic, pItems, sizeof(Type), itemCount);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##Free(structName* pDarray)\
{\
    _memRealloc(&pDarray->_generic, 0, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##Reserve(structName* pDarray, size_t newCapacity)\
{\
    if (pDarray->_capacity >= newCapacity) return;\
    _memRealloc(&pDarray->_generic, newCapacity, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##ShrinkToFit(structName* pDarray)\
{\
    _memRealloc(&pDarray->_generic, pDarray->_count, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##ResizeZ(structName* pDarray, size_t newSize)\
{\
    _memResize(&pDarray->_generic, newSize, &_darray##structName##Null, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##ResizeV(structName* pDarray, size_t newSize, const Type vItem)\
{\
    _memResize(&pDarray->_generic, newSize, &vItem, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##ResizeR(structName* pDarray, size_t newSize, const Type* pItem)\
{\
    _memResize(&pDarray->_generic, newSize, pItem, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##ResizeDirectly(structName* pDarray, size_t newSize)\
{\
    if (newSize > pDarray->_capacity)\
    {\
        fprintf(stderr, "newSize = %zu > pDarray->_capacity given to %s for Darray %p\n",\
                        newSize, __func__, pDarray);\
        exit(EXIT_FAILURE);\
    }\
    pDarray->_count = newSize;\
}\
\
/* Addition */\
static inline _mem_FUNC_ATTRIB void fPrefix##AppendV(structName* pDarray, const Type vItem)\
{\
    _memInsert(&pDarray->_generic, pDarray->_count, &vItem, sizeof(Type), 1);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##AppendR(structName* pDarray, const Type* pItem)\
{\
    _memInsert(&pDarray->_generic, pDarray->_count, pItem, sizeof(Type), 1);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##AppendD(structName* pDstDarray, const structName* pAppendedDarray)\
{\
    _memInsert(&pDstDarray->_generic, pDstDarray->_count,\
                pAppendedDarray->_pBuffer, sizeof(Type), pAppendedDarray->_count);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##AppendA(structName* pDarray, const Type* pItems, size_t count)\
{\
    _memInsert(&pDarray->_generic, pDarray->_count,\
                pItems, sizeof(Type), count);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##InsertV(structName* pDarray, size_t index, const Type vItem)\
{\
    _memInsert(&pDarray->_generic, index, &vItem, sizeof(Type), 1);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##InsertR(structName* pDarray, size_t index, const Type* pItem)\
{\
    _memInsert(&pDarray->_generic, index, pItem, sizeof(Type), 1);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##InsertD(structName* pDstDarray, size_t index, const structName* pInsertedDarray)\
{\
    _memInsert(&pDstDarray->_generic, index,\
                    pInsertedDarray->_pBuffer, sizeof(Type), pInsertedDarray->_count);\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##InsertA(structName* pDarray, size_t index, const Type* pItems, size_t count)\
{\
    _memInsert(&pDarray->_generic, index, pItems, sizeof(Type), count);\
}\
\
/* Erasure */\
static inline _mem_FUNC_ATTRIB void fPrefix##Clear(structName* pDarray)\
{\
    pDarray->_count = 0;\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##Erase(structName* pDarray, size_t startI, size_t count)\
{\
    _memErase(&pDarray->_generic, startI, count, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##EraseStart(structName* pDarray, size_t count)\
{\
    _memErase(&pDarray->_generic, 0, count, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB void fPrefix##EraseEnd(structName* pDarray, size_t count)\
{\
    _memErase(&pDarray->_generic, pDarray->_count -count, count, sizeof(Type));\
}\
\
/* Getters */\
static inline _mem_FUNC_ATTRIB Type* fPrefix##Data(structName* pDarray)\
{\
    return pDarray->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB const Type* fPrefix##DataConst(const structName* pDarray)\
{\
    return (const Type*)pDarray->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB size_t fPrefix##Size(const structName* pDarray)\
{\
    return pDarray->_count;\
}\
static inline _mem_FUNC_ATTRIB size_t fPrefix##Capacity(const structName* pDarray)\
{\
    return pDarray->_capacity;\
}\
static inline _mem_FUNC_ATTRIB bool fPrefix##Empty(const structName* pDarray)\
{\
    return (pDarray->_count == 0);\
}\
static inline _mem_FUNC_ATTRIB Type fPrefix##First(const structName* pDarray)\
{\
    return *(Type*)_memAt(&pDarray->_generic, 0, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB Type fPrefix##Last(const structName* pDarray)\
{\
    return *(Type*)_memAt(&pDarray->_generic, pDarray->_count -1, sizeof(Type));\
}

DARRAY_DEF(Dbyte, dbyte, byte_t)
DARRAY_DEF(Dbool, dbool, bool)
DARRAY_DEF(Dsize, dsize, size_t)
