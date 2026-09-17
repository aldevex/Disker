// Dynamic Array
#pragma once
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef unsigned char byte_t;

// Can't name it "_Darray" or compiler headers could possibly fuck the code
typedef struct _darrayBytes
{
    byte_t* _pBytes;
    size_t _itemsCapacity, _itemsCount;
} _darrayBytes;

// 3 in 1: allocates, reallocates, or frees the array buffer
void _darrayRealloc(_darrayBytes* pDarray, size_t newCapacity, size_t itemSize);

void _darrayMakeFill(_darrayBytes* pDarray, size_t count, const void* pItem, size_t itemSize);
void _darrayMakeCopy(_darrayBytes* pDarray, const void* pItems, size_t itemSize, size_t itemCount);

void _darrayInsert(_darrayBytes* pDarray, size_t index, const void* pItems, size_t itemSize, size_t itemCount);
void _darrayErase(_darrayBytes* pDarray, size_t startI, size_t count, size_t itemSize);
void* _darrayAt(const _darrayBytes* pDarray, size_t index, size_t itemSize);



#define _darrayToGeneric(pTypedDarray)\
    (_darrayBytes)\
    {\
        ._pBytes = (byte_t*)((pTypedDarray)->_pBuffer),\
        ._itemsCapacity = (pTypedDarray)->_itemsCapacity,\
        ._itemsCount = (pTypedDarray)->_itemsCount\
    }
#define _darrayToTyped(vGenericDarray, structName, ItemType)\
    (structName)\
    {\
        ._pBuffer = (ItemType*)vGenericDarray._pBytes,\
        ._itemsCapacity = vGenericDarray._itemsCapacity,\
        ._itemsCount = vGenericDarray._itemsCount\
    }

#define DARRAY_DEF(structName, fPrefix, Type)\
\
typedef struct structName\
{\
    Type* _pBuffer;\
    size_t _itemsCapacity, _itemsCount;\
} structName;\
\
/* Creation, destruction, and generic allocation and size changes */\
static inline structName fPrefix##MakeFillV(size_t count, const Type vItem)\
{\
    _darrayBytes generic = {0};\
    _darrayMakeFill(&generic, count, &vItem, sizeof(Type));\
    return _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##FillV(structName* pDarray, size_t count, const Type vItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayMakeFill(&generic, count, &vItem, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline structName fPrefix##MakeFillR(size_t count, const Type* pItem)\
{\
    _darrayBytes generic = {0};\
    _darrayMakeFill(&generic, count, pItem, sizeof(Type));\
    return _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##FillR(structName* pDarray, size_t count, const Type* pItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayMakeFill(&generic, count, pItem, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline structName fPrefix##MakeCopyD(const structName* pSrcDarray)\
{\
    _darrayBytes generic = {0};\
    _darrayMakeCopy(&generic, pSrcDarray->_pBuffer, sizeof(Type), pSrcDarray->_itemsCount);\
    return _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##CopyD(structName* pDarray, const structName* pSrcDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayMakeCopy(&generic, pSrcDarray->_pBuffer, sizeof(Type), pSrcDarray->_itemsCount);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline structName fPrefix##MakeCopyA(const Type* pItems, size_t itemCount)\
{\
    _darrayBytes generic = {0};\
    _darrayMakeCopy(&generic, pItems, sizeof(Type), itemCount);\
    return _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##CopyA(structName* pDarray, const Type* pItems, size_t itemCount)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayMakeCopy(&generic, pItems, sizeof(Type), itemCount);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##Free(structName* pDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayRealloc(&generic, 0, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##Reserve(structName* pDarray, size_t newCapacity)\
{\
    if (pDarray->_itemsCapacity >= newCapacity) return;\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayRealloc(&generic, newCapacity, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##ShrinkToFit(structName* pDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayRealloc(&generic, generic._itemsCount, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##ResizeZ(structName* pDarray, size_t newSize)\
{\
    _darrayBytes ogCopy = _darrayToGeneric(pDarray);\
    _darrayBytes generic = ogCopy;\
    ogCopy._pBytes = NULL;\
    _darrayRealloc(&generic, newSize, sizeof(Type));\
    /* Fill new area with zeroes */\
    if (newSize > ogCopy._itemsCount)\
        memset(generic._pBytes +(ogCopy._itemsCount * sizeof(Type)), 0,\
                (newSize -ogCopy._itemsCount) *sizeof(Type));\
    generic._itemsCount = newSize;\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##ResizeV(structName* pDarray, size_t newSize, const Type vItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayBytes ogCopy = generic;\
    ogCopy._pBytes = NULL;\
    _darrayRealloc(&generic, newSize, sizeof(Type));\
    /*Using _darrayMakeFill() for fast memcpy algorithm*/\
    _darrayBytes copyInfo = {\
        ._pBytes = generic._pBytes +(ogCopy._itemsCount *sizeof(Type)),\
        ._itemsCapacity = newSize -ogCopy._itemsCount,\
        ._itemsCount = newSize -ogCopy._itemsCount\
    };\
    _darrayMakeFill(&copyInfo, newSize -ogCopy._itemsCount, &vItem, sizeof(Type));\
    generic._itemsCount = newSize;\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##ResizeR(structName* pDarray, size_t newSize, const Type* pItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayBytes ogCopy = generic;\
    ogCopy._pBytes = NULL;\
    _darrayRealloc(&generic, newSize, sizeof(Type));\
    /*Using _darrayMakeFill() for fast memcpy algorithm*/\
    _darrayBytes copyInfo = {\
        ._pBytes = generic._pBytes +(ogCopy._itemsCount *sizeof(Type)),\
        ._itemsCapacity = newSize -ogCopy._itemsCount,\
        ._itemsCount = newSize -ogCopy._itemsCount\
    };\
    _darrayMakeFill(&copyInfo, newSize -ogCopy._itemsCount, pItem, sizeof(Type));\
    generic._itemsCount = newSize;\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
\
/* Addition */\
static inline void fPrefix##AppendV(structName* pDarray, const Type vItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayInsert(&generic, generic._itemsCount, &vItem, sizeof(Type), 1);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##AppendR(structName* pDarray, const Type* pItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayInsert(&generic, generic._itemsCount, pItem, sizeof(Type), 1);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##AppendD(structName* pDstDarray, const structName* pAppendedDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDstDarray);\
    _darrayInsert(&generic, generic._itemsCount,\
                    pAppendedDarray->_pBuffer, sizeof(Type), pAppendedDarray->_itemsCount);\
    *pDstDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##AppendA(structName* pDarray, const Type* pItems, size_t count)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayInsert(&generic, generic._itemsCount, pItems, sizeof(Type), count);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##InsertV(structName* pDarray, size_t index, const Type vItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayInsert(&generic, index, &vItem, sizeof(Type), 1);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##InsertR(structName* pDarray, size_t index, const Type* pItem)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayInsert(&generic, index, pItem, sizeof(Type), 1);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##InsertD(structName* pDstDarray, size_t index, const structName* pInsertedDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDstDarray);\
    _darrayInsert(&generic, index,\
                    pInsertedDarray->_pBuffer, sizeof(Type), pInsertedDarray->_itemsCount);\
    *pDstDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##InsertA(structName* pDarray, size_t index, const Type* pItems, size_t count)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayInsert(&generic, index, pItems, sizeof(Type), count);\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
\
/* Erasure */\
static inline void fPrefix##Clear(structName* pDarray)\
{\
    pDarray->_itemsCount = 0;\
}\
static inline void fPrefix##Erase(structName* pDarray, size_t startI, size_t count)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayErase(&generic, startI, count, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##EraseStart(structName* pDarray, size_t count)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayErase(&generic, 0, count, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
static inline void fPrefix##EraseEnd(structName* pDarray, size_t count)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    _darrayErase(&generic, generic._itemsCount -count, count, sizeof(Type));\
    *pDarray = _darrayToTyped(generic, structName, Type);\
}\
\
/* Getters */\
static inline Type* fPrefix##Data(structName* pDarray)\
{\
    return pDarray->_pBuffer;\
}\
static inline const Type* fPrefix##DataConst(const structName* pDarray)\
{\
    return (const Type*)pDarray->_pBuffer;\
}\
static inline size_t fPrefix##Capacity(const structName* pDarray)\
{\
    return pDarray->_itemsCapacity;\
}\
static inline size_t fPrefix##Size(const structName* pDarray)\
{\
    return pDarray->_itemsCount;\
}\
static inline bool fPrefix##Empty(const structName* pDarray)\
{\
    return (pDarray->_itemsCount == 0);\
}\
static inline Type fPrefix##First(const structName* pDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    return *(Type*)_darrayAt(&generic, 0, sizeof(Type));\
}\
static inline Type fPrefix##Last(const structName* pDarray)\
{\
    _darrayBytes generic = _darrayToGeneric(pDarray);\
    return *(Type*)_darrayAt(&generic, pDarray->_itemsCount -1, sizeof(Type));\
}

DARRAY_DEF(Dbyte, dbyte, byte_t)
DARRAY_DEF(Dbool, dbool, bool)
