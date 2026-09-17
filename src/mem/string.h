#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "./darray.h"

#define _stringToDarray(pString, structName, Type)\
    (structName)\
    {\
        ._pBuffer = (Type*)((pString)->_pBuffer),\
        ._itemsCapacity = (pString)->_itemsCapacity,\
        ._itemsCount = (pString)->_itemsCount\
    }
#define _stringFromDarray(vDarray, structName, Type)\
    (structName)\
    {\
        ._pBuffer = (Type*)vDarray._pBuffer,\
        ._itemsCapacity = vDarray._itemsCapacity,\
        ._itemsCount = vDarray._itemsCount\
    }
#define _stringDarrayFromNT(pNTString, structName, Type, sizeFinderFuncWithOneArgumentOnlyAStringPointerOkay)\
    (structName)\
    {\
        ._pBuffer = (Type*)(pNTString),\
        ._itemsCapacity = sizeFinderFuncWithOneArgumentOnlyAStringPointerOkay(pNTString),\
        ._itemsCount = sizeFinderFuncWithOneArgumentOnlyAStringPointerOkay(pNTString)\
    }
#define _stringDarrayFromPS(pText, size, structName, Type)\
    (structName)\
    {\
        ._pBuffer = (Type*)(pText),\
        ._itemsCapacity = size,\
        ._itemsCount = size\
    }

#define _stringViewToDarray(pView, structName, Type)\
    (structName)\
    {\
        ._pBuffer = (Type*)((pView)->_pBuffer),\
        ._itemsCapacity = (pView)->_itemsCount,\
        ._itemsCount = (pView)->_itemsCount\
    }
#define _stringViewFromString(pString, structName, Type)\
    (structName)\
    {\
        ._pBuffer = (Type*)((pString)->_pBuffer),\
        ._itemsCount = (pString)->_itemsCount,\
        ._terminated = true\
    }
#define _stringViewFromNT(pNTString, structName, Type, sizeFinderFuncWithOneArgumentOnlyAStringPointerOkay)\
    (structName)\
    {\
        ._pBuffer = (Type*)(pNTString),\
        ._itemsCount = sizeFinderFuncWithOneArgumentOnlyAStringPointerOkay(pNTString),\
        ._terminated = true\
    }
#define _stringViewFromPS(pText, size, structName, Type)\
    (structName)\
    {\
        ._pBuffer = (Type*)(pText),\
        ._itemsCount = size,\
        ._terminated = false\
    }

// This macro doesn't implement trim, strlen, isspace, and encoding conversion functions,
//   you should make them outside the macro

#define STRING_DEF(stringName, stringPrefix, viewName, viewPrefix, darrayName, darrayPrefix,\
                    Type, funcGetNTCodePointCount)\
\
DARRAY_DEF(darrayName, darrayPrefix, Type)\
typedef struct stringName\
{\
    Type* _pBuffer;\
    size_t _itemsCapacity, _itemsCount;\
} stringName;\
\
typedef struct viewName\
{\
    const Type* _pBuffer;\
    size_t _itemsCount;\
    bool _terminated;\
} viewName;\
\
/* Creation and assignment */\
static inline stringName stringPrefix##MakeFillV(size_t count, const Type vItem)\
{\
    darrayName darray = {0};\
    darrayPrefix##FillR(&darray, count, &vItem);\
    return _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##FillV(stringName* pString, size_t count, const Type vItem)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##FillR(&darray, count, &vItem);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline stringName stringPrefix##MakeFillR(size_t count, const Type* pItem)\
{\
    darrayName darray = {0};\
    darrayPrefix##FillR(&darray, count, pItem);\
    return _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##FillR(stringName* pString, size_t count, const Type* pItem)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##FillR(&darray, count, pItem);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline stringName stringPrefix##MakeCopyS(const stringName* pString)\
{\
    darrayName dstDarray = {0};\
    const darrayName srcDarray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    return _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##CopyS(stringName* pDstString, const stringName* pSrcString)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringToDarray(pSrcString, darrayName, Type);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline stringName stringPrefix##MakeCopyVw(const viewName* pView)\
{\
    darrayName dstDarray = {0};\
    const darrayName srcDarray = _stringViewToDarray(pView, darrayName, Type);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    return _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##CopyVw(stringName* pDstString, const viewName* pSrcView)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringViewToDarray(pSrcView, darrayName, Type);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline stringName stringPrefix##MakeCopyNT(const Type* pNTtring)\
{\
    darrayName dstDarray = {0};\
    const darrayName srcDarray = _stringDarrayFromNT(pNTtring, darrayName, Type, funcGetNTCodePointCount);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    return _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##CopyNT(stringName* pDstString, const Type* pSrcNTtring)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringDarrayFromNT(pSrcNTtring, darrayName, Type, funcGetNTCodePointCount);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline stringName stringPrefix##MakeCopyPS(const Type* pText, size_t size)\
{\
    darrayName dstDarray = {0};\
    const darrayName srcDarray = _stringDarrayFromPS(pText, size, darrayName, Type);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    return _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##CopyPS(stringName* pDstString, const Type* pText, size_t size)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringDarrayFromPS(pText, size, darrayName, Type);\
    darrayPrefix##CopyD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
\
/* Destruction, and generic allocation and size changes */\
static inline void stringPrefix##Free(stringName* pString)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##Free(&darray);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##Reserve(stringName* pString, size_t newCapacity)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##Reserve(&darray, newCapacity);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##ShrinkToFit(stringName* pString)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##ShrinkToFit(&darray);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##ResizeZ(stringName* pString, size_t newSize)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##ResizeZ(&darray, newSize);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##ResizeV(stringName* pString, size_t newSize, const Type vItem)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##ResizeR(&darray, newSize, &vItem);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##ResizeR(stringName* pString, size_t newSize, const Type* pItem)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##ResizeR(&darray, newSize, pItem);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
\
/* Addition */\
static inline void stringPrefix##AppendCU(stringName* pDstString, Type appendedCodeUnit)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    darrayPrefix##AppendV(&dstDarray, (Type)appendedCodeUnit);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##AppendS(stringName* pDstString, const stringName* pAppendedString)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringToDarray(pAppendedString, darrayName, Type);\
    darrayPrefix##AppendD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##AppendVw(stringName* pDstString, const viewName* pAppendedView)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringViewToDarray(pAppendedView, darrayName, Type);\
    darrayPrefix##AppendD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##AppendNT(stringName* pDstString, const Type* pAppendedNTtring)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringDarrayFromNT(pAppendedNTtring, darrayName, Type, funcGetNTCodePointCount);\
    darrayPrefix##AppendD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##AppendPS(stringName* pDstString, const Type* pAppendedText, size_t size)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringDarrayFromPS(pAppendedText, size, darrayName, Type);\
    darrayPrefix##AppendD(&dstDarray, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##InsertCU(stringName* pDstString, size_t index, Type insertedCodeUnit)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    darrayPrefix##InsertV(&dstDarray, index, (Type)insertedCodeUnit);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##InsertS(stringName* pDstString, size_t index, const stringName* pInsertedString)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringToDarray(pInsertedString, darrayName, Type);\
    darrayPrefix##InsertD(&dstDarray, index, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##InsertVw(stringName* pDstString, size_t index, const viewName* pInsertedView)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringViewToDarray(pInsertedView, darrayName, Type);\
    darrayPrefix##InsertD(&dstDarray, index, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##InsertNT(stringName* pDstString, size_t index, const Type* pInsertedNTtring)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringDarrayFromNT(pInsertedNTtring, darrayName, Type, funcGetNTCodePointCount);\
    darrayPrefix##InsertD(&dstDarray, index, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
static inline void stringPrefix##InsertPS(stringName* pDstString, size_t index, const Type* pInsertedText, size_t size)\
{\
    darrayName dstDarray = _stringToDarray(pDstString, darrayName, Type);\
    const darrayName srcDarray = _stringDarrayFromPS(pInsertedText, size, darrayName, Type);\
    darrayPrefix##InsertD(&dstDarray, index, &srcDarray);\
    *pDstString = _stringFromDarray(dstDarray, stringName, Type);\
}\
/* Erasure */\
\
static inline void stringPrefix##Clear(stringName* pString)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##Clear(&darray);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##Erase(stringName* pString, size_t startI, size_t count)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##Erase(&darray, startI, count);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##EraseStart(stringName* pString, size_t count)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##EraseStart(&darray, count);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
static inline void stringPrefix##EraseEnd(stringName* pString, size_t count)\
{\
    darrayName darray = _stringToDarray(pString, darrayName, Type);\
    darrayPrefix##EraseEnd(&darray, count);\
    *pString = _stringFromDarray(darray, stringName, Type);\
}\
/* Getters */\
\
static inline Type* stringPrefix##Data(stringName* pString)\
{\
    return pString->_pBuffer;\
}\
static inline const Type* stringPrefix##DataConst(const stringName* pString)\
{\
    return (const Type*)pString->_pBuffer;\
}\
static inline size_t stringPrefix##Size(const stringName* pString)\
{\
    return pString->_itemsCount;\
}\
static inline size_t stringPrefix##Capacity(const stringName* pString)\
{\
    return pString->_itemsCapacity;\
}\
static inline bool stringPrefix##Empty(const stringName* pString)\
{\
    return (pString->_itemsCount == 0);\
}\
static inline const Type* stringPrefix##NT(const stringName* pString)\
{\
    return pString->_pBuffer;\
}\
static inline stringName stringPrefix##SubStr(const stringName* pString, size_t startI, size_t count)\
{\
    /* Fabricate view for make-copy */\
    viewName subView = (viewName){\
        ._pBuffer = pString->_pBuffer +startI,\
        ._itemsCount = count,\
        ._terminated = false\
    };\
    return stringPrefix##MakeCopyVw(&subView);\
}\
/* Comparison */\
\
static inline bool stringPrefix##CmpS(const stringName* pA, const stringName* pB)\
{\
    if (pA->_itemsCount != pB->_itemsCount) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##StartsWithS(const stringName* pString, const stringName* pPrefix)\
{\
    if (pString->_itemsCount < pPrefix->_itemsCount) return false;\
    for (size_t i = 0; i < pPrefix->_itemsCount; i++)\
        if (pString->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##CmpVw(const stringName* pA, const viewName* pB)\
{\
    if (pA->_itemsCount != pB->_itemsCount) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##StartsWithVw(const stringName* pString, const viewName* pPrefix)\
{\
    if (pString->_itemsCount < pPrefix->_itemsCount) return false;\
    for (size_t i = 0; i < pPrefix->_itemsCount; i++)\
        if (pString->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##CmpNT(const stringName* pA, const Type* pB)\
{\
    const size_t bSize = funcGetNTCodePointCount(pB);\
    if (pA->_itemsCount != bSize) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##StartsWithNT(const stringName* pString, const Type* pPrefix)\
{\
    const size_t prefixSize = funcGetNTCodePointCount(pPrefix);\
    if (pString->_itemsCount < prefixSize) return false;\
    for (size_t i = 0; i < prefixSize; i++)\
        if (pString->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##CmpPS(const stringName* pA, const Type* pB, size_t size)\
{\
    if (pA->_itemsCount != size) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline bool stringPrefix##StartsWithPS(const stringName* pString, const Type* pPrefix, size_t size)\
{\
    if (pString->_itemsCount < size) return false;\
    for (size_t i = 0; i < size; i++)\
        if (pString->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
\
\
\
\
\
/* View functions */\
\
\
\
\
\
/* Creation and assignment */\
static inline viewName viewPrefix##MakeCopyS(const stringName* pString)\
{\
    return _stringViewFromString(pString, viewName, Type);\
}\
static inline void viewPrefix##CopyS(viewName* pDstView, const stringName* pSrcString)\
{\
    *pDstView = _stringViewFromString(pSrcString, viewName, Type);\
}\
static inline viewName viewPrefix##MakeCopyVw(const viewName* pView)\
{\
    return *pView;\
}\
static inline void viewPrefix##CopyVw(viewName* pDstView, const viewName* pSrcView)\
{\
    *pDstView = *pSrcView;\
}\
static inline viewName viewPrefix##MakeCopyNT(const Type* pNTtring)\
{\
    return _stringViewFromNT(pNTtring, viewName, Type, funcGetNTCodePointCount);\
}\
static inline void viewPrefix##CopyNT(viewName* pDstView, const Type* pSrcNTtring)\
{\
    *pDstView = _stringViewFromNT(pSrcNTtring, viewName, Type, funcGetNTCodePointCount);\
}\
static inline viewName viewPrefix##MakeCopyPS(const Type* pText, size_t size)\
{\
    return _stringViewFromPS(pText, size, viewName, Type);\
}\
static inline void viewPrefix##CopyPS(viewName* pDstView, const Type* pText, size_t size)\
{\
    *pDstView = _stringViewFromPS(pText, size, viewName, Type);\
}\
/* Size changes */\
\
static inline void viewPrefix##ResizeZ(viewName* pView, size_t newSize)\
{\
    if (newSize > pView->_itemsCount)\
    {\
        fprintf(stderr, "newSize = %zu > pView->_itemsCount given to %s for view %p\n",\
                        newSize, __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    if (newSize == pView->_itemsCount) return;\
    pView->_itemsCount = newSize;\
    pView->_terminated = false;\
}\
/* Erasure */\
\
static inline void viewPrefix##Clear(viewName* pView)\
{\
    pView->_pBuffer = NULL;\
    pView->_itemsCount = 0;\
    pView->_terminated = false;\
}\
static inline void viewPrefix##EraseStart(viewName* pView, size_t count)\
{\
    pView->_pBuffer += count;\
    pView->_itemsCount -= count;\
}\
static inline void viewPrefix##EraseEnd(viewName* pView, size_t count)\
{\
    if (count == 0) return;\
    pView->_itemsCount -= count;\
    pView->_terminated = false;\
}\
/* Getters */\
\
static inline const Type* viewPrefix##Data(const viewName* pView)\
{\
    return pView->_pBuffer;\
}\
static inline size_t viewPrefix##Size(const viewName* pView)\
{\
    return pView->_itemsCount;\
}\
static inline bool viewPrefix##Empty(const viewName* pView)\
{\
    return (pView->_itemsCount == 0);\
}\
static inline bool viewPrefix##Terminated(const viewName* pView)\
{\
    return pView->_terminated;\
}\
static inline viewName viewPrefix##SubStr(const viewName* pView, size_t startI, size_t count)\
{\
    if (startI +count > pView->_itemsCount)\
    {\
        fprintf(stderr, "startI +count > pView->_itemsCount "\
                        "(startI = %zu count = %zu) "\
                        "given to %s for view %p\n",\
                        startI, count, __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    /* Create and return new view */\
    return (viewName){\
        ._pBuffer = pView->_pBuffer +startI,\
        ._itemsCount = count,\
        ._terminated = (pView->_terminated && startI +count == pView->_itemsCount)\
    };\
}\
/* Comparison */\
\
static inline bool viewPrefix##CmpVw(const viewName* pA, const viewName* pB)\
{\
    if (pA->_itemsCount != pB->_itemsCount) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##StartsWithVw(const viewName* pView, const viewName* pPrefix)\
{\
    if (pView->_itemsCount < pPrefix->_itemsCount) return false;\
    for (size_t i = 0; i < pPrefix->_itemsCount; i++)\
        if (pView->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##CmpS(const viewName* pA, const stringName* pB)\
{\
    if (pA->_itemsCount != pB->_itemsCount) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##StartsWithS(const viewName* pView, const stringName* pPrefix)\
{\
    if (pView->_itemsCount < pPrefix->_itemsCount) return false;\
    for (size_t i = 0; i < pPrefix->_itemsCount; i++)\
        if (pView->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##CmpNT(const viewName* pA, const Type* pB)\
{\
    const size_t bSize = funcGetNTCodePointCount(pB);\
    if (pA->_itemsCount != bSize) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##StartsWithNT(const viewName* pView, const Type* pPrefix)\
{\
    const size_t prefixSize = funcGetNTCodePointCount(pPrefix);\
    if (pView->_itemsCount < prefixSize) return false;\
    for (size_t i = 0; i < prefixSize; i++)\
        if (pView->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##CmpPS(const viewName* pA, const Type* pB, size_t size)\
{\
    if (pA->_itemsCount != size) return false;\
    for (size_t i = 0; i < pA->_itemsCount; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline bool viewPrefix##StartsWithPS(const viewName* pView, const Type* pPrefix, size_t size)\
{\
    if (pView->_itemsCount < size) return false;\
    for (size_t i = 0; i < size; i++)\
        if (pView->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
\
/* String find (requires view comparison functions) */\
static inline size_t stringPrefix##FindPS(const stringName* pString, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pString->_itemsCount) return SIZE_MAX;\
    for (size_t i = 0; i <= pString->_itemsCount -querySize; i++)\
    {\
        /* Fabricate view for current index search */\
        viewName currentView = (viewName){\
            ._pBuffer = pString->_pBuffer +i,\
            ._itemsCount = querySize,\
            ._terminated = false\
        };\
        if (viewPrefix##CmpPS(&currentView, pQuery, querySize))\
            return i;\
    }\
    return SIZE_MAX;\
}\
static inline size_t stringPrefix##FindNT(const stringName* pString, const Type* pQuery)\
{\
    return stringPrefix##FindPS(pString, pQuery, funcGetNTCodePointCount(pQuery));\
}\
static inline size_t stringPrefix##FindS(const stringName* pString, const stringName* pQuery)\
{\
    return stringPrefix##FindPS(pString, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
static inline size_t stringPrefix##FindVw(const stringName* pString, const viewName* pQuery)\
{\
    return stringPrefix##FindPS(pString, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
static inline size_t stringPrefix##RevFindPS(const stringName* pString, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pString->_itemsCount) return SIZE_MAX;\
    for (size_t i = pString->_itemsCount -querySize; i != SIZE_MAX; i--)\
    {\
        /* Fabricate view for current index search */\
        viewName currentView = (viewName){\
            ._pBuffer = pString->_pBuffer +i,\
            ._itemsCount = querySize,\
            ._terminated = false\
        };\
        if (viewPrefix##CmpPS(&currentView, pQuery, querySize))\
            return i;\
    }\
    return SIZE_MAX;\
}\
static inline size_t stringPrefix##RevFindNT(const stringName* pString, const Type* pQuery)\
{\
    return stringPrefix##RevFindPS(pString, pQuery, funcGetNTCodePointCount(pQuery));\
}\
static inline size_t stringPrefix##RevFindS(const stringName* pString, const stringName* pQuery)\
{\
    return stringPrefix##RevFindPS(pString, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
static inline size_t stringPrefix##RevFindVw(const stringName* pString, const viewName* pQuery)\
{\
    return stringPrefix##RevFindPS(pString, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
/* View find (requires view comparison functions) */\
static inline size_t viewPrefix##FindPS(const viewName* pView, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pView->_itemsCount) return SIZE_MAX;\
    for (size_t i = 0; i <= pView->_itemsCount -querySize; i++)\
    {\
        /* Fabricate view for current index search */\
        viewName currentView = (viewName){\
            ._pBuffer = pView->_pBuffer +i,\
            ._itemsCount = querySize,\
            ._terminated = false\
        };\
        if (viewPrefix##CmpPS(&currentView, pQuery, querySize))\
            return i;\
    }\
    return SIZE_MAX;\
}\
static inline size_t viewPrefix##FindNT(const viewName* pView, const Type* pQuery)\
{\
    return viewPrefix##FindPS(pView, pQuery, funcGetNTCodePointCount(pQuery));\
}\
static inline size_t viewPrefix##FindS(const viewName* pView, const stringName* pQuery)\
{\
    return viewPrefix##FindPS(pView, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
static inline size_t viewPrefix##FindVw(const viewName* pView, const viewName* pQuery)\
{\
    return viewPrefix##FindPS(pView, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
static inline size_t viewPrefix##RevFindPS(const viewName* pView, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pView->_itemsCount) return SIZE_MAX;\
    for (size_t i = pView->_itemsCount -querySize; i != SIZE_MAX; i--)\
    {\
        /* Fabricate view for current index search */\
        viewName currentView = (viewName){\
            ._pBuffer = pView->_pBuffer +i,\
            ._itemsCount = querySize,\
            ._terminated = false\
        };\
        if (viewPrefix##CmpPS(&currentView, pQuery, querySize))\
            return i;\
    }\
    return SIZE_MAX;\
}\
static inline size_t viewPrefix##RevFindNT(const viewName* pView, const Type* pQuery)\
{\
    return viewPrefix##RevFindPS(pView, pQuery, funcGetNTCodePointCount(pQuery));\
}\
static inline size_t viewPrefix##RevFindS(const viewName* pView, const stringName* pQuery)\
{\
    return viewPrefix##RevFindPS(pView, pQuery->_pBuffer, pQuery->_itemsCount);\
}\
static inline size_t viewPrefix##RevFindVw(const viewName* pView, const viewName* pQuery)\
{\
    return viewPrefix##RevFindPS(pView, pQuery->_pBuffer, pQuery->_itemsCount);\
}


typedef char UTF8_t;
typedef uint16_t UTF16_t;

static inline size_t countNTSize8(const UTF8_t* pString)
{
    size_t cuCount = 0;
    while (pString[cuCount] != (UTF8_t)0) cuCount++;
    return cuCount;
}
// Used in isSpace
static inline size_t countNTSizeLimited8(const UTF8_t* pString, size_t max)
{
    size_t cuCount = 0;
    while (cuCount < max && pString[cuCount] != (UTF8_t)0) cuCount++;
    return cuCount;
}

static inline size_t countNTSize16(const UTF16_t* pString)
{
    size_t cuCount = 0;
    while (pString[cuCount] != (UTF16_t)0) cuCount++;
    return cuCount;
}
// Used in isSpace
static inline size_t countNTSizeLimited16(const UTF16_t* pString, size_t max)
{
    size_t cuCount = 0;
    while (cuCount < max && pString[cuCount] != (UTF16_t)0) cuCount++;
    return cuCount;
}

STRING_DEF(String8, string8, View8, view8, _stringDarray8, _stringDarray8,
            UTF8_t, countNTSize8)
STRING_DEF(String16, string16, View16, view16, _stringDarray16, _stringDarray16,
            UTF16_t, countNTSize16)



static const UTF16_t _stringWhitespaceChars16[] = {
    u'\t',     // U+0009 Horizontal Tab
    u'\n',     // U+000A Line Feed
    u'\v',     // U+000B Vertical Tab
    u'\f',     // U+000C Form Feed
    u'\r',     // U+000D Carriage Return
    u' ',      // U+0020 Space
    u'\x0085', // U+0085 Next Line (NEL)
    u'\u00A0', // U+00A0 No-Break Space
    u'\u1680', // U+1680 Ogham Space Mark
    u'\u2000', // U+2000 En Quad
    u'\u2001', // U+2001 Em Quad
    u'\u2002', // U+2002 En Space
    u'\u2003', // U+2003 Em Space
    u'\u2004', // U+2004 Three-Per-Em Space
    u'\u2005', // U+2005 Four-Per-Em Space
    u'\u2006', // U+2006 Six-Per-Em Space
    u'\u2007', // U+2007 Figure Space
    u'\u2008', // U+2008 Punctuation Space
    u'\u2009', // U+2009 Thin Space
    u'\u200A', // U+200A Hair Space
    u'\u2028', // U+2028 Line Separator
    u'\u2029', // U+2029 Paragraph Separator
    u'\u202F', // U+202F Narrow No-Break Space
    u'\u205F', // U+205F Medium Mathematical Space
    u'\u3000'  // U+3000 Ideographic Space
};
static inline bool isSpace16(const UTF16_t codeUnit)
{
    const UTF16_t* pWhitespaceCharArray = _stringWhitespaceChars16;
    const size_t whitespaceCharCount = sizeof(_stringWhitespaceChars16) / sizeof(_stringWhitespaceChars16[0]);

    /* Loop over all whitespace characters */
    for (size_t wsCharI = 0; wsCharI < (whitespaceCharCount); wsCharI++)
    {
        if (codeUnit == pWhitespaceCharArray[wsCharI])
            return true;
    }
    return false;
}
static inline void string16TrimStart(String16* pString)
{
    size_t count = 0;
    while (count < pString->_itemsCount && isSpace16(pString->_pBuffer[count]))
        count++;
    _stringDarray16 darray = _stringToDarray(pString, _stringDarray16, UTF16_t);
    _stringDarray16EraseStart(&darray, count);
    *pString = _stringFromDarray(darray, String16, UTF16_t);
}
static inline void view16TrimStart(View16* pView)
{
    size_t count = 0;
    while (count < pView->_itemsCount && isSpace16(pView->_pBuffer[count]))
        count++;
    pView->_pBuffer += count;
    pView->_itemsCount -= count;
}
static inline void string16TrimEnd(String16* pString)
{
    size_t count = 0;
    while (count < pString->_itemsCount && isSpace16(pString->_pBuffer[pString->_itemsCount -count -1]))
        count++;
    _stringDarray16 darray = _stringToDarray(pString, _stringDarray16, UTF16_t);
    _stringDarray16EraseEnd(&darray, count);
    *pString = _stringFromDarray(darray, String16, UTF16_t);
}
static inline void view16TrimEnd(View16* pView)
{
    size_t count = 0;
    while (count < pView->_itemsCount && isSpace16(pView->_pBuffer[pView->_itemsCount -count -1]))
        count++;
    pView->_itemsCount -= count;
    if (count != 0) pView->_terminated = false;
}
static inline void string16Trim(String16* pString)
{
    string16TrimStart(pString);
    string16TrimEnd(pString);
}
static inline void view16Trim(View16* pView)
{
    view16TrimStart(pView);
    view16TrimEnd(pView);
}

static const UTF8_t* _stringWhitespaceStrings8[] = {
    u8"\t",       // U+0009 Horizontal Tab
    u8"\n",       // U+000A Line Feed
    u8"\v",       // U+000B Vertical Tab
    u8"\f",       // U+000C Form Feed
    u8"\r",       // U+000D Carriage Return
    u8" ",        // U+0020 Space
    u8"\xC2\x85", // U+0085 Next Line (NEL)
    u8"\u00A0",   // U+00A0 No-Break Space
    u8"\u1680",   // U+1680 Ogham Space Mark
    u8"\u2000",   // U+2000 En Quad
    u8"\u2001",   // U+2001 Em Quad
    u8"\u2002",   // U+2002 En Space
    u8"\u2003",   // U+2003 Em Space
    u8"\u2004",   // U+2004 Three-Per-Em Space
    u8"\u2005",   // U+2005 Four-Per-Em Space
    u8"\u2006",   // U+2006 Six-Per-Em Space
    u8"\u2007",   // U+2007 Figure Space
    u8"\u2008",   // U+2008 Punctuation Space
    u8"\u2009",   // U+2009 Thin Space
    u8"\u200A",   // U+200A Hair Space
    u8"\u2028",   // U+2028 Line Separator
    u8"\u2029",   // U+2029 Paragraph Separator
    u8"\u202F",   // U+202F Narrow No-Break Space
    u8"\u205F",   // U+205F Medium Mathematical Space
    u8"\u3000"    // U+3000 Ideographic Space
};
static inline bool isSpacePS8(const UTF8_t* pCodeUnits, size_t availableCount, size_t* pSpaceSize_optional)
{
    const UTF8_t** pWhitespaceStringArray = _stringWhitespaceStrings8;
    const size_t whitespaceStringCount = sizeof(_stringWhitespaceStrings8) / sizeof(_stringWhitespaceStrings8[0]);

    /* Loop over all whitespace strings */
    for (size_t wsStrI = 0; wsStrI < (whitespaceStringCount); wsStrI++)
    {
        const size_t wsStrSize = countNTSizeLimited8(pWhitespaceStringArray[wsStrI], 3);
        /* Too little code units for comparison */
        if (availableCount < wsStrSize) continue;
        /* Fabricate view for comparison */
        View8 codeUnitsView = (View8){
            ._pBuffer = pCodeUnits,
            ._itemsCount = availableCount,
            ._terminated = false
        };
        /* Compare */
        if (view8StartsWithPS(&codeUnitsView, pWhitespaceStringArray[wsStrI], wsStrSize))
        {
            if (pSpaceSize_optional != NULL) *pSpaceSize_optional = wsStrSize;
            return true;
        }
    }
    return false;
}
static inline bool isSpaceNT8(const UTF8_t* pCodeUnits, size_t* pSpaceSize_optional)
{
    return isSpacePS8(pCodeUnits, countNTSizeLimited8(pCodeUnits, 3), pSpaceSize_optional);
}
static inline void view8TrimStart(View8* pView)
{
    size_t count = 0;
    size_t wsSize = 0; // Amount of code units in the found whitespace
    while (count < pView->_itemsCount
    && isSpacePS8(&pView->_pBuffer[count], pView->_itemsCount, &wsSize))
        count += wsSize;
    pView->_pBuffer += count;
    pView->_itemsCount -= count;
}
static inline void string8TrimStart(String8* pString)
{
    size_t count = 0;
    size_t wsSize = 0; // Amount of code units in the found whitespace
    while (count < pString->_itemsCount
    && isSpacePS8(&pString->_pBuffer[count], pString->_itemsCount, &wsSize))
        count += wsSize;
    _stringDarray8 darray = _stringToDarray(pString, _stringDarray8, UTF8_t);
    _stringDarray8EraseStart(&darray, count);
    *pString = _stringFromDarray(darray, String8, UTF8_t);
}
static inline void view8TrimEnd(View8* pView)
{
    size_t count = 0;
    while (count < pView->_itemsCount)
    {
        // Amount of code units in the found whitespace
        size_t wsSize = 0;
        // Try the last code unit
        if (isSpacePS8(&pView->_pBuffer[pView->_itemsCount -count -1],
                        pView->_itemsCount -count,
                        NULL))
        {
            count += 1;
        }
        // Try the last two code units
        else if (count +2 <= pView->_itemsCount
                && isSpacePS8(&pView->_pBuffer[pView->_itemsCount -count -2],
                                pView->_itemsCount -count,
                                &wsSize)
                && wsSize == 2)
        {
            count += 2;
        }
        // Try the last three code units
        else if (count +3 <= pView->_itemsCount
                && isSpacePS8(&pView->_pBuffer[pView->_itemsCount -count -3],
                                pView->_itemsCount -count,
                                &wsSize)
                && wsSize == 3)
        {
            count += 3;
        }
        // No whitespace at the ending was found
        else break;
    }
    pView->_itemsCount -= count;
    if (count != 0) pView->_terminated = false;
}
static inline void string8TrimEnd(String8* pString)
{
    /* Fabricate view for trim (I'm not going to copy 500000000 lines of code) */
    View8 stringView = (View8){
        ._pBuffer = pString->_pBuffer,
        ._itemsCount = pString->_itemsCount,
        ._terminated = false
    };
    view8TrimEnd(&stringView);
    pString->_itemsCount = stringView._itemsCount;
}
static inline void view8Trim(View8* pView)
{
    view8TrimStart(pView);
    view8TrimEnd(pView);
}
static inline void string8Trim(String8* pString)
{
    string8TrimStart(pString);
    string8TrimEnd(pString);
}



// static inline bool validateUTF8Sequence(const UTF8_t* pString, const size_t size,
//                                         size_t* pLastCodePointSize_optional)
// {
//     bool foundBadCUs = false;
//     for (size_t srcI = 0; srcI < size; )
//     {
//         // Validate first code unit isn't 11111xxx (code point above U+10FFFF)
//         if (pString[srcI +0] > 0xF4)
//         {
//             foundBadCUs = true;
//             srcI += 1;
//             continue;
//         }
//         // One code unit
//         if ((pString[srcI +0] & 0b10000000) == 0b0)
//         {
//             srcI += 1;
//         }
//         // Two code units
//         else if ((pString[srcI +0] & 0b11100000) == 0b11000000)
//         {
//             // Validate second code unit
//             if ((srcI +1) >= size8
//             || (pSrc8[srcI +1] & 0b11000000) != 0b10000000)
//             {
//                 string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
//                 foundBadCUs = true;
//                 srcI += 1;
//                 continue;
//             }
//             string16AppendCU(&result,
//                 ((UTF16_t)(pSrc8[srcI +0] & 0b00011111) << 6)
//                 + (UTF16_t)(pSrc8[srcI +1] & 0b00111111)
//             );
//             srcI += 2;
//         }
//         // Three code units
//         else if ((pSrc8[srcI +0] & 0b11110000) == 0b11100000)
//         {
//             // Validate second and third code units
//             if ((srcI +2) >= size8
//             || (pSrc8[srcI +1] & 0b11000000) != 0b10000000
//             || (pSrc8[srcI +2] & 0b11000000) != 0b10000000)
//             {
//                 string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
//                 foundBadCUs = true;
//                 srcI += 1;
//                 continue;
//             }
//             string16AppendCU(&result,
//                 ((UTF16_t)(pSrc8[srcI +0] & 0b00001111) << 12)
//                 + ((UTF16_t)(pSrc8[srcI +1] & 0b00111111) << 6)
//                 +(UTF16_t)(pSrc8[srcI +2] & 0b00111111)
//             );
//             srcI += 3;
//         }
//         // Four code units
//         else if ((pSrc8[srcI +0] & 0b11111000) == 0b11110000) 
//         {
//             // Validate second, third, and fourth code units
//             if ((srcI +3) >= size8
//             || (pSrc8[srcI +1] & 0b11000000) != 0b10000000
//             || (pSrc8[srcI +2] & 0b11000000) != 0b10000000
//             || (pSrc8[srcI +3] & 0b11000000) != 0b10000000)
//             {
//                 string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
//                 foundBadCUs = true;
//                 srcI += 1;
//                 continue;
//             }
//             uint32_t conversionValue =
//                 ((uint32_t)(pSrc8[srcI +0] & 0b00000111) << 18)
//                 + ((uint32_t)(pSrc8[srcI +1] & 0b00111111) << 12)
//                 + ((uint32_t)(pSrc8[srcI +2] & 0b00111111) << 6)
//                 + (uint32_t)(pSrc8[srcI +3] & 0b00111111)
//             - 0x10000;
//             // High surrogate
//             string16AppendCU(&result,
//                 (UTF16_t)(0xD800 + (conversionValue >> 10))
//             );
//             // Low surrogate
//             string16AppendCU(&result,
//                 (UTF16_t)(0xDC00 + (conversionValue & 0b1111111111))
//             );
//             srcI += 4;
//         }
//         // Invalid first code unit
//         else
//         {
//             string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
//             foundBadCUs = true;
//             srcI += 1;
//             continue;
//         }
//     }
// }
// static inline bool validateUTF16CP()
// {

// }



// Returns true on success
static inline bool _string_UTF8to16(const UTF8_t* pSrc8, const size_t size8, String16* pNewString16)
{
    String16 result = {0};
    // Initial result size would be AT LEAST, MINIMUM equal to UTF-8 code units
    // And string funcs would reallocate if required, so all is good
    string16Reserve(&result, size8);

    bool foundBadCUs = false;
    for (size_t srcI = 0; srcI < size8; )
    {
        // One code unit
        if ((pSrc8[srcI +0] & 0b10000000) == 0b0)
        {
            string16AppendCU(&result, (UTF16_t)pSrc8[srcI +0]);
            srcI += 1;
        }
        // Two code units
        else if ((pSrc8[srcI +0] & 0b11100000) == 0b11000000)
        {
            // Validate second code unit
            if ((srcI +1) >= size8
            || (pSrc8[srcI +1] & 0b11000000) != 0b10000000)
            {
                string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            string16AppendCU(&result,
                ((UTF16_t)(pSrc8[srcI +0] & 0b00011111) << 6)
                + (UTF16_t)(pSrc8[srcI +1] & 0b00111111)
            );
            srcI += 2;
        }
        // Three code units
        else if ((pSrc8[srcI +0] & 0b11110000) == 0b11100000)
        {
            // Validate second and third code units
            if ((srcI +2) >= size8
            || (pSrc8[srcI +1] & 0b11000000) != 0b10000000
            || (pSrc8[srcI +2] & 0b11000000) != 0b10000000)
            {
                string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            string16AppendCU(&result,
                ((UTF16_t)(pSrc8[srcI +0] & 0b00001111) << 12)
                + ((UTF16_t)(pSrc8[srcI +1] & 0b00111111) << 6)
                +(UTF16_t)(pSrc8[srcI +2] & 0b00111111)
            );
            srcI += 3;
        }
        // Four code units
        else if ((pSrc8[srcI +0] & 0b11111000) == 0b11110000) 
        {
            // Validate second, third, and fourth code units
            if ((srcI +3) >= size8
            || (pSrc8[srcI +1] & 0b11000000) != 0b10000000
            || (pSrc8[srcI +2] & 0b11000000) != 0b10000000
            || (pSrc8[srcI +3] & 0b11000000) != 0b10000000)
            {
                string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            uint32_t conversionValue =
                ((uint32_t)(pSrc8[srcI +0] & 0b00000111) << 18)
                + ((uint32_t)(pSrc8[srcI +1] & 0b00111111) << 12)
                + ((uint32_t)(pSrc8[srcI +2] & 0b00111111) << 6)
                + (uint32_t)(pSrc8[srcI +3] & 0b00111111)
            - 0x10000;
            // High surrogate
            string16AppendCU(&result,
                (UTF16_t)(0xD800 + (conversionValue >> 10))
            );
            // Low surrogate
            string16AppendCU(&result,
                (UTF16_t)(0xDC00 + (conversionValue & 0b1111111111))
            );
            srcI += 4;
        }
        // Invalid first code unit
        else
        {
            string16AppendCU(&result, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
            foundBadCUs = true;
            srcI += 1;
            continue;
        }
    }
    *pNewString16 = result;
    return !foundBadCUs;
}
// Returns true on success
static inline bool _string_UTF16to8(const UTF16_t* pSrc16, const size_t size16, String8* pNewString8)
{
    String8 result = {0};
    // Initial result size would be AT LEAST, MINIMUM equal to UTF-16 code units
    // And string funcs would reallocate if required, so all is good
    string8Reserve(&result, size16);

    bool foundBadCUs = false;
    for (size_t srcI = 0; srcI < size16; )
    {
        // High surrogate
        if (pSrc16[srcI +0] >= 0xD800 && pSrc16[srcI +0] <= 0xDBFF)
        {
            // Validate second code unit (must be a low surrogate)
            if (pSrc16[srcI +1] < 0xDC00 || pSrc16[srcI +1] > 0xDFFF)
            {
                string8AppendPS(&result, (UTF8_t[]){0xEF, 0xBF, 0xBD}, 3); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            uint32_t conversionValue =
                (((uint32_t)(pSrc16[srcI +0] & 0b1111111111)) << 10) // High surrogate
                + (uint32_t)(pSrc16[srcI +1] & 0b1111111111) // Low surrogate
            + 0x10000U;
            string8AppendCU(&result,
                (UTF8_t)(((conversionValue >> 18) & 0b00000111) | 0b11110000)
            );
            string8AppendCU(&result,
                (UTF8_t)(((conversionValue >> 12) & 0b00111111) | 0b10000000)
            );
            string8AppendCU(&result,
                (UTF8_t)(((conversionValue >> 6) & 0b00111111) | 0b10000000)
            );
            string8AppendCU(&result,
                (UTF8_t)((conversionValue & 0b00111111) | 0b10000000)
            );
            srcI += 2;
        }
        // BMP code unit, not a high surrogate in a pair
        // Validate (musn't be a low surrogate, and is in the BMP)
        else if ((pSrc16[srcI +0] < 0xDC00 || pSrc16[srcI +0] > 0xDFFF) && pSrc16[srcI +0] < 0x10000)
        {
            // Convert to one UTF-8 code unit
            if (pSrc16[srcI +0] <= 0x007F)
            {
                string8AppendCU(&result,
                    (UTF8_t)pSrc16[srcI +0]
                );
            }
            // Convert to two UTF-8 code units
            else if (pSrc16[srcI +0] <= 0x07FF)
            {
                string8AppendCU(&result,
                    (UTF8_t)(((pSrc16[srcI +0] >> 6) & 0b0011111) | 0b11000000)
                );
                string8AppendCU(&result,
                    (UTF8_t)((pSrc16[srcI +0] & 0b0111111) | 0b10000000)
                );
            }
            // Convert to three UTF-8 code units
            else if (pSrc16[srcI +0] <= 0xFFFF)
            {
                string8AppendCU(&result,
                    (UTF8_t)(((pSrc16[srcI +0] >> 12) & 0b00001111) | 0b11100000)
                );
                string8AppendCU(&result,
                    (UTF8_t)(((pSrc16[srcI +0] >> 6) & 0b00111111) | 0b10000000)
                );
                string8AppendCU(&result,
                    (UTF8_t)((pSrc16[srcI +0] & 0b00111111) | 0b10000000)
                );
            }
            srcI += 1;
        }
        // Invalid first code unit
        else
        {
            string8AppendPS(&result, (UTF8_t[]){0xEF, 0xBF, 0xBD}, 3); // U+FFFD Replacement Character
            foundBadCUs = true;
            srcI += 1;
            continue;
        }
    }
    *pNewString8 = result;
    return !foundBadCUs;
}

static inline String8 string8MakeCopyS16(const String16* pSrcString, bool* pValidInput_optional)
{
    String8 result = {0};
    bool isValid = _string_UTF16to8(pSrcString->_pBuffer, pSrcString->_itemsCount, &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string8CopyS16(String8* pDstString, const String16* pSrcString)
{
    return _string_UTF16to8(pSrcString->_pBuffer, pSrcString->_itemsCount, pDstString);
}
static inline String8 string8MakeCopyVw16(const View16* pSrcView, bool* pValidInput_optional)
{
    String8 result = {0};
    bool isValid = _string_UTF16to8(pSrcView->_pBuffer, pSrcView->_itemsCount, &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string8CopyVw16(String8* pDstString, const View16* pSrcView)
{
    return _string_UTF16to8(pSrcView->_pBuffer, pSrcView->_itemsCount, pDstString);
}
static inline String8 string8MakeCopyNT16(const UTF16_t* pSrcText, bool* pValidInput_optional)
{
    String8 result = {0};
    bool isValid = _string_UTF16to8(pSrcText, countNTSize16(pSrcText), &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string8CopyNT16(String8* pDstString, const UTF16_t* pSrcText)
{
    return _string_UTF16to8(pSrcText, countNTSize16(pSrcText), pDstString);
}
static inline String8 string8MakeCopyPS16(const UTF16_t* pSrcText, size_t size, bool* pValidInput_optional)
{
    String8 result = {0};
    bool isValid = _string_UTF16to8(pSrcText, size, &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string8CopyPS16(String8* pDstString, const UTF16_t* pSrcText, size_t size)
{
    return _string_UTF16to8(pSrcText, size, pDstString);
}

static inline String16 string16MakeCopyS8(const String8* pSrcString, bool* pValidInput_optional)
{
    String16 result = {0};
    bool isValid = _string_UTF8to16(pSrcString->_pBuffer, pSrcString->_itemsCount, &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string16CopyS8(String16* pDstString, const String8* pSrcString)
{
    return _string_UTF8to16(pSrcString->_pBuffer, pSrcString->_itemsCount, pDstString);
}
static inline String16 string16MakeCopyVw8(const View8* pSrcView, bool* pValidInput_optional)
{
    String16 result = {0};
    bool isValid = _string_UTF8to16(pSrcView->_pBuffer, pSrcView->_itemsCount, &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string16CopyVw8(String16* pDstString, const View8* pSrcView)
{
    return _string_UTF8to16(pSrcView->_pBuffer, pSrcView->_itemsCount, pDstString);
}
static inline String16 string16MakeCopyNT8(const UTF8_t* pSrcText, bool* pValidInput_optional)
{
    String16 result = {0};
    bool isValid = _string_UTF8to16(pSrcText, countNTSize8(pSrcText), &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string16CopyNT8(String16* pDstString, const UTF8_t* pSrcText)
{
    return _string_UTF8to16(pSrcText, countNTSize8(pSrcText), pDstString);
}
static inline String16 string16MakeCopyPS8(const UTF8_t* pSrcText, size_t size, bool* pValidInput_optional)
{
    String16 result = {0};
    bool isValid = _string_UTF8to16(pSrcText, size, &result);
    if (pValidInput_optional != NULL) *pValidInput_optional = isValid;
    return result;
}
static inline bool string16CopyPS8(String16* pDstString, const UTF8_t* pSrcText, size_t size)
{
    return _string_UTF8to16(pSrcText, size, pDstString);
}



DARRAY_DEF(Dstr, dstr, String8)
DARRAY_DEF(Dview, dview, View8)



static inline Dstr view8SplitPS(const View8* pView, const UTF8_t* pDelimiter, size_t size,
                                    bool discardEmptyResults, bool contiguousAreOne)
{
    if (size == 0)
    {
        fprintf(stderr, "size = 0 given to %s for view %p\n",
                        __func__, pView);
        exit(EXIT_FAILURE);
    }
    Dstr result = {0};
    size_t startI = 0;
    for (size_t i = 0; i <= view8Size(pView); )
    {
        /* Fabricate view for comparison */
        View8 currentView = (View8){
            ._pBuffer = pView->_pBuffer +i,
            ._itemsCount = pView->_itemsCount -i,
            ._terminated = pView->_terminated
        };
        bool firstCPIsDelimiter = view8StartsWithPS(&currentView, pDelimiter, size);
        if (firstCPIsDelimiter || i == view8Size(pView))
        {
            if (i -startI != 0 || !discardEmptyResults)
                dstrAppendV(&result, string8MakeCopyPS(pView->_pBuffer +startI, i -startI));
            i += size;
            startI = i;
            if (contiguousAreOne && firstCPIsDelimiter)
            {
                while (i +size <= view8Size(pView))
                {
                    /* Fabricate view for comparison (again) */
                    currentView = (View8){
                        ._pBuffer = pView->_pBuffer +i,
                        ._itemsCount = pView->_itemsCount -i,
                        ._terminated = pView->_terminated
                    };
                    if (!view8StartsWithPS(&currentView, pDelimiter, size)) break;
                    else i += size;
                }
                startI = i;
            }
        }
        else i++;
    }
    return result;
}
static inline Dstr view8SplitNT(const View8* pView, const UTF8_t* pDelimiter,
                                    bool discardEmptyResults, bool contiguousAreOne)
{
    return view8SplitPS(pView, pDelimiter, countNTSize8(pDelimiter), discardEmptyResults, contiguousAreOne);
}

static inline Dstr string8SplitPS(const String8* pString, const UTF8_t* pDelimiter, size_t size,
                                    bool discardEmptyResults, bool contiguousAreOne)
{
    View8 view = view8MakeCopyS(pString);
    return view8SplitPS(&view, pDelimiter, size, discardEmptyResults, contiguousAreOne);
}
static inline Dstr string8SplitNT(const String8* pString, const UTF8_t* pDelimiter,
                                    bool discardEmptyResults, bool contiguousAreOne)
{
    View8 view = view8MakeCopyS(pString);
    return view8SplitPS(&view, pDelimiter, countNTSize8(pDelimiter), discardEmptyResults, contiguousAreOne);
}
