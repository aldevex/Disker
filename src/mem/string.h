#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <uchar.h>
#include <string.h>
#include "./core.h"
#include "./darray.h"

// This macro doesn't implement encoding-specific functions,
//   those must be defined outside the macro

// Views assume a constant pointed-to string
//   and assume null-termination if copied from
//   the end of a null-terminated string object or text pointer

// NF means Not Found
#define STRING_NF ((size_t)(-1))

#define STRING_DEF(stringName, stringPrefix, viewName, viewPrefix,\
                    genericSuffix, Type)\
\
static const Type _stringNull##genericSuffix = (Type)0;\
\
/* String count and capacity don't include null terminator, core functions must handle it */\
typedef union stringName\
{\
    _memGS _generic;\
    struct {\
        Type* _pBuffer;\
        size_t _count;\
        size_t _capacity;\
    };\
} stringName;\
\
typedef union viewName\
{\
    _memGS _generic;\
    struct {\
        const Type* _pBuffer;\
        size_t _count;\
        bool _terminated;\
    };\
} viewName;\
\
/* Generic non-object-focused utils */\
static inline _mem_FUNC_ATTRIB size_t countNTSize##genericSuffix(const Type* pString)\
{\
    size_t cuCount = 0;\
    while (pString[cuCount] != (Type)0) cuCount++;\
    return cuCount;\
}\
/* Used in isSpace */\
static inline _mem_FUNC_ATTRIB size_t countNTSizeLimited##genericSuffix(const Type* pString, size_t maximum)\
{\
    size_t cuCount = 0;\
    while (cuCount < maximum && pString[cuCount] != (Type)0) cuCount++;\
    return cuCount;\
}\
\
/* Creation and assignment */\
static inline _mem_FUNC_ATTRIB void stringPrefix##FillV(stringName* pString, size_t count, const Type vItem)\
{\
    _memStrFill(&pString->_generic, count, &vItem, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##FillR(stringName* pString, size_t count, const Type* pItem)\
{\
    _memStrFill(&pString->_generic, count, pItem, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##CopyS(stringName* pDstString, const stringName* pSrcString)\
{\
    _memStrCopy(&pDstString->_generic, pSrcString->_pBuffer, sizeof(Type), pSrcString->_count,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##CopyVw(stringName* pDstString, const viewName* pSrcView)\
{\
    _memStrCopy(&pDstString->_generic, pSrcView->_pBuffer, sizeof(Type), pSrcView->_count,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##CopyNT(stringName* pDstString, const Type* pSrcNTtring)\
{\
    _memStrCopy(&pDstString->_generic, pSrcNTtring, sizeof(Type), countNTSize##genericSuffix(pSrcNTtring),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##CopyPS(stringName* pDstString, const Type* pText, size_t size)\
{\
    _memStrCopy(&pDstString->_generic, pText, sizeof(Type), size,\
                &_stringNull##genericSuffix);\
}\
\
/* Destruction, and generic allocation and size changes */\
static inline _mem_FUNC_ATTRIB void stringPrefix##Free(stringName* pString)\
{\
    _memStrRealloc(&pString->_generic, 0, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##Reserve(stringName* pString, size_t newCapacity)\
{\
    if (pString->_capacity >= newCapacity) return;\
    _memStrRealloc(&pString->_generic, newCapacity, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##ShrinkToFit(stringName* pString)\
{\
    _memStrRealloc(&pString->_generic, pString->_count, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##ResizeZ(stringName* pString, size_t newSize)\
{\
    _memStrResize(&pString->_generic, newSize, &_stringNull##genericSuffix, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##ResizeV(stringName* pString, size_t newSize, const Type vItem)\
{\
    _memStrResize(&pString->_generic, newSize, &vItem, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##ResizeR(stringName* pString, size_t newSize, const Type* pItem)\
{\
    _memStrResize(&pString->_generic, newSize, pItem, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##ResizeDirectly(stringName* pString, size_t newSize)\
{\
    if (newSize > pString->_capacity)\
        _memStrRealloc(&pString->_generic, newSize, sizeof(Type),\
                &_stringNull##genericSuffix);\
    pString->_count = newSize;\
}\
\
/* Addition */\
static inline _mem_FUNC_ATTRIB void stringPrefix##AppendCU(stringName* pDstString, Type codeUnit)\
{\
    _memStrInsert(&pDstString->_generic, pDstString->_count, &codeUnit, sizeof(Type), 1,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##AppendS(stringName* pDstString, const stringName* pAppendedString)\
{\
    _memStrInsert(&pDstString->_generic, pDstString->_count,\
                pAppendedString->_pBuffer, sizeof(Type), pAppendedString->_count,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##AppendVw(stringName* pDstString, const viewName* pAppendedView)\
{\
    _memStrInsert(&pDstString->_generic, pDstString->_count,\
                pAppendedView->_pBuffer, sizeof(Type), pAppendedView->_count,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##AppendNT(stringName* pDstString, const Type* pAppendedNTtring)\
{\
    _memStrInsert(&pDstString->_generic, pDstString->_count,\
                pAppendedNTtring, sizeof(Type), countNTSize##genericSuffix(pAppendedNTtring),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##AppendPS(stringName* pDstString, const Type* pAppendedText, size_t size)\
{\
    _memStrInsert(&pDstString->_generic, pDstString->_count,\
                pAppendedText, sizeof(Type), size,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##InsertCU(stringName* pDstString, size_t index, Type codeUnit)\
{\
    _memStrInsert(&pDstString->_generic, index, &codeUnit, sizeof(Type), 1,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##InsertS(stringName* pDstString, size_t index, const stringName* pInsertedString)\
{\
    _memStrInsert(&pDstString->_generic, index,\
                pInsertedString->_pBuffer, sizeof(Type), pInsertedString->_count,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##InsertVw(stringName* pDstString, size_t index, const viewName* pInsertedView)\
{\
    _memStrInsert(&pDstString->_generic, index,\
                pInsertedView->_pBuffer, sizeof(Type), pInsertedView->_count,\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##InsertNT(stringName* pDstString, size_t index, const Type* pInsertedNTtring)\
{\
    _memStrInsert(&pDstString->_generic, index,\
                pInsertedNTtring, sizeof(Type), countNTSize##genericSuffix(pInsertedNTtring),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##InsertPS(stringName* pDstString, size_t index, const Type* pInsertedText, size_t size)\
{\
    _memStrInsert(&pDstString->_generic, index,\
                pInsertedText, sizeof(Type), size,\
                &_stringNull##genericSuffix);\
}\
/* Erasure */\
\
static inline _mem_FUNC_ATTRIB void stringPrefix##Clear(stringName* pString)\
{\
    _memStrResize(&pString->_generic, 0, &_stringNull##genericSuffix, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##Erase(stringName* pString, size_t startI, size_t count)\
{\
    _memStrErase(&pString->_generic, startI, count, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##EraseStart(stringName* pString, size_t count)\
{\
    _memStrErase(&pString->_generic, 0, count, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
static inline _mem_FUNC_ATTRIB void stringPrefix##EraseEnd(stringName* pString, size_t count)\
{\
    _memStrErase(&pString->_generic, pString->_count -count, count, sizeof(Type),\
                &_stringNull##genericSuffix);\
}\
/* Getters */\
\
static inline _mem_FUNC_ATTRIB Type* stringPrefix##Data(stringName* pString)\
{\
    return pString->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB const Type* stringPrefix##DataConst(const stringName* pString)\
{\
    return (const Type*)pString->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##Size(const stringName* pString)\
{\
    return pString->_count;\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##Capacity(const stringName* pString)\
{\
    return pString->_capacity;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##Empty(const stringName* pString)\
{\
    return (pString->_count == 0);\
}\
static inline _mem_FUNC_ATTRIB Type stringPrefix##First(const stringName* pString)\
{\
    return *(Type*)_memAt(&pString->_generic, 0, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB Type stringPrefix##Last(const stringName* pString)\
{\
    return *(Type*)_memAt(&pString->_generic, pString->_count -1, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB const Type* stringPrefix##NT(const stringName* pString)\
{\
    if (pString->_pBuffer == NULL) return &_stringNull##genericSuffix;\
    else return pString->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB viewName stringPrefix##SubStr(const stringName* pString, size_t startI, size_t count)\
{\
    /* Check bad arguments */\
    if (startI +count > pString->_count)\
    {\
        fprintf(stderr, "startI +count = %zu > pString->_count "\
                        "(startI = %zu count = %zu) "\
                        "given to %s for String %p\n",\
                        startI +count, startI, count,\
                        __func__, pString);\
        exit(EXIT_FAILURE);\
    }\
    return (viewName){\
        ._pBuffer = pString->_pBuffer +startI,\
        ._count = count,\
        ._terminated = (startI +count == pString->_count)\
    };\
}\
/* Comparison */\
\
static inline _mem_FUNC_ATTRIB bool stringPrefix##CmpS(const stringName* pA, const stringName* pB)\
{\
    if (pA->_count != pB->_count) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##StartsWithS(const stringName* pString, const stringName* pPrefix)\
{\
    if (pString->_count < pPrefix->_count) return false;\
    for (size_t i = 0; i < pPrefix->_count; i++)\
        if (pString->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##CmpVw(const stringName* pA, const viewName* pB)\
{\
    if (pA->_count != pB->_count) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##StartsWithVw(const stringName* pString, const viewName* pPrefix)\
{\
    if (pString->_count < pPrefix->_count) return false;\
    for (size_t i = 0; i < pPrefix->_count; i++)\
        if (pString->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##CmpNT(const stringName* pA, const Type* pB)\
{\
    const size_t bSize = countNTSize##genericSuffix(pB);\
    if (pA->_count != bSize) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##StartsWithNT(const stringName* pString, const Type* pPrefix)\
{\
    const size_t prefixSize = countNTSize##genericSuffix(pPrefix);\
    if (pString->_count < prefixSize) return false;\
    for (size_t i = 0; i < prefixSize; i++)\
        if (pString->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##CmpPS(const stringName* pA, const Type* pB, size_t size)\
{\
    if (pA->_count != size) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool stringPrefix##StartsWithPS(const stringName* pString, const Type* pPrefix, size_t size)\
{\
    if (pString->_count < size) return false;\
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
static inline _mem_FUNC_ATTRIB viewName viewPrefix##MakeCopyS(const stringName* pString)\
{\
    return (viewName){\
        ._pBuffer = pString->_pBuffer,\
        ._count = pString->_count,\
        ._terminated = (pString->_pBuffer != NULL)\
    };\
}\
static inline _mem_FUNC_ATTRIB void viewPrefix##CopyS(viewName* pDstView, const stringName* pSrcString)\
{\
    *pDstView = (viewName){\
        ._pBuffer = pSrcString->_pBuffer,\
        ._count = pSrcString->_count,\
        ._terminated = (pSrcString->_pBuffer != NULL)\
    };\
}\
static inline _mem_FUNC_ATTRIB viewName viewPrefix##MakeCopyVw(const viewName* pView)\
{\
    return *pView;\
}\
static inline _mem_FUNC_ATTRIB void viewPrefix##CopyVw(viewName* pDstView, const viewName* pSrcView)\
{\
    *pDstView = *pSrcView;\
}\
static inline _mem_FUNC_ATTRIB viewName viewPrefix##MakeCopyNT(const Type* pNTtring)\
{\
    return (viewName){\
        ._pBuffer = pNTtring,\
        ._count = countNTSize##genericSuffix(pNTtring),\
        ._terminated = true\
    };\
}\
static inline _mem_FUNC_ATTRIB void viewPrefix##CopyNT(viewName* pDstView, const Type* pSrcNTtring)\
{\
    *pDstView = (viewName){\
        ._pBuffer = pSrcNTtring,\
        ._count = countNTSize##genericSuffix(pSrcNTtring),\
        ._terminated = true\
    };\
}\
static inline _mem_FUNC_ATTRIB viewName viewPrefix##MakeCopyPS(const Type* pText, size_t size)\
{\
    return (viewName){\
        ._pBuffer = pText,\
        ._count = size,\
        ._terminated = false\
    };\
}\
static inline _mem_FUNC_ATTRIB void viewPrefix##CopyPS(viewName* pDstView, const Type* pText, size_t size)\
{\
    *pDstView = (viewName){\
        ._pBuffer = pText,\
        ._count = size,\
        ._terminated = false\
    };\
}\
/* Size changes */\
\
static inline _mem_FUNC_ATTRIB void viewPrefix##Resize(viewName* pView, size_t newSize)\
{\
    if (newSize > pView->_count)\
    {\
        fprintf(stderr, "newSize = %zu > pView->_count given to %s for View %p\n",\
                        newSize, __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    if (newSize == pView->_count) return;\
    pView->_count = newSize;\
    pView->_terminated = false;\
}\
/* Erasure */\
\
static inline _mem_FUNC_ATTRIB void viewPrefix##Clear(viewName* pView)\
{\
    *pView = (viewName){\
        ._pBuffer = NULL,\
        ._count = 0,\
        ._terminated = false\
    };\
}\
static inline _mem_FUNC_ATTRIB void viewPrefix##EraseStart(viewName* pView, size_t count)\
{\
    if (count > pView->_count)\
    {\
        fprintf(stderr, "count = %zu > pView->_count given to %s for View %p\n",\
                        count, __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    pView->_pBuffer += count;\
    pView->_count -= count;\
}\
static inline _mem_FUNC_ATTRIB void viewPrefix##EraseEnd(viewName* pView, size_t count)\
{\
    if (count > pView->_count)\
    {\
        fprintf(stderr, "count = %zu > pView->_count given to %s for View %p\n",\
                        count, __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    pView->_count -= count;\
    pView->_terminated = false;\
}\
/* Getters */\
\
static inline _mem_FUNC_ATTRIB const Type* viewPrefix##Data(const viewName* pView)\
{\
    return pView->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##Size(const viewName* pView)\
{\
    return pView->_count;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##Empty(const viewName* pView)\
{\
    return (pView->_count == 0);\
}\
static inline _mem_FUNC_ATTRIB Type viewPrefix##First(const viewName* pView)\
{\
    return *(Type*)_memAt(&pView->_generic, 0, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB Type viewPrefix##Last(const viewName* pView)\
{\
    return *(Type*)_memAt(&pView->_generic, pView->_count -1, sizeof(Type));\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##NT(const viewName* pView)\
{\
    if (!pView->_terminated)\
    {\
        fprintf(stderr, "pView->_terminated == false given to %s for View %p\n",\
                        __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    return pView->_pBuffer;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##Terminated(const viewName* pView)\
{\
    return pView->_terminated;\
}\
static inline _mem_FUNC_ATTRIB viewName viewPrefix##SubStr(const viewName* pView, size_t startI, size_t count)\
{\
    /* Check bad arguments */\
    if (startI +count > pView->_count)\
    {\
        fprintf(stderr, "startI +count = %zu > pView->_count "\
                        "(startI = %zu count = %zu) "\
                        "given to %s for View %p\n",\
                        startI +count, startI, count,\
                        __func__, pView);\
        exit(EXIT_FAILURE);\
    }\
    return (viewName){\
        ._pBuffer = pView->_pBuffer +startI,\
        ._count = count,\
        ._terminated = (pView->_terminated\
            && startI +count == pView->_count)\
    };\
}\
/* Comparison */\
\
static inline _mem_FUNC_ATTRIB bool viewPrefix##CmpVw(const viewName* pA, const viewName* pB)\
{\
    if (pA->_count != pB->_count) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##StartsWithVw(const viewName* pView, const viewName* pPrefix)\
{\
    if (pView->_count < pPrefix->_count) return false;\
    for (size_t i = 0; i < pPrefix->_count; i++)\
        if (pView->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##CmpS(const viewName* pA, const stringName* pB)\
{\
    if (pA->_count != pB->_count) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##StartsWithS(const viewName* pView, const stringName* pPrefix)\
{\
    if (pView->_count < pPrefix->_count) return false;\
    for (size_t i = 0; i < pPrefix->_count; i++)\
        if (pView->_pBuffer[i] != pPrefix->_pBuffer[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##CmpNT(const viewName* pA, const Type* pB)\
{\
    const size_t bSize = countNTSize##genericSuffix(pB);\
    if (pA->_count != bSize) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##StartsWithNT(const viewName* pView, const Type* pPrefix)\
{\
    const size_t prefixSize = countNTSize##genericSuffix(pPrefix);\
    if (pView->_count < prefixSize) return false;\
    for (size_t i = 0; i < prefixSize; i++)\
        if (pView->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##CmpPS(const viewName* pA, const Type* pB, size_t size)\
{\
    if (pA->_count != size) return false;\
    for (size_t i = 0; i < pA->_count; i++)\
        if (pA->_pBuffer[i] != pB[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB bool viewPrefix##StartsWithPS(const viewName* pView, const Type* pPrefix, size_t size)\
{\
    if (pView->_count < size) return false;\
    for (size_t i = 0; i < size; i++)\
        if (pView->_pBuffer[i] != pPrefix[i]) return false;\
    return true;\
}\
\
/* String find */\
static inline _mem_FUNC_ATTRIB bool _stringDirectCmp##genericSuffix(const Type* pA, const Type* pB, size_t bothSize)\
{\
    for (size_t i = 0; i < bothSize; i++)\
        if (pA[i] != pB[i]) return false;\
    return true;\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##FindPS(const stringName* pString, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pString->_count) return STRING_NF;\
    for (size_t i = 0; i <= pString->_count -querySize; i++)\
    {\
        if (_stringDirectCmp##genericSuffix(pString->_pBuffer +i, pQuery, querySize))\
            return i;\
    }\
    return STRING_NF;\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##FindNT(const stringName* pString, const Type* pQuery)\
{\
    return stringPrefix##FindPS(pString, pQuery, countNTSize##genericSuffix(pQuery));\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##FindS(const stringName* pString, const stringName* pQuery)\
{\
    return stringPrefix##FindPS(pString, pQuery->_pBuffer, pQuery->_count);\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##FindVw(const stringName* pString, const viewName* pQuery)\
{\
    return stringPrefix##FindPS(pString, pQuery->_pBuffer, pQuery->_count);\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##RevFindPS(const stringName* pString, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pString->_count) return STRING_NF;\
    for (size_t i = pString->_count -querySize; i != (size_t)(-1); i--)\
    {\
        if (_stringDirectCmp##genericSuffix(pString->_pBuffer +i, pQuery, querySize))\
            return i;\
    }\
    return STRING_NF;\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##RevFindNT(const stringName* pString, const Type* pQuery)\
{\
    return stringPrefix##RevFindPS(pString, pQuery, countNTSize##genericSuffix(pQuery));\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##RevFindS(const stringName* pString, const stringName* pQuery)\
{\
    return stringPrefix##RevFindPS(pString, pQuery->_pBuffer, pQuery->_count);\
}\
static inline _mem_FUNC_ATTRIB size_t stringPrefix##RevFindVw(const stringName* pString, const viewName* pQuery)\
{\
    return stringPrefix##RevFindPS(pString, pQuery->_pBuffer, pQuery->_count);\
}\
/* View find (requires view comparison functions) */\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##FindPS(const viewName* pView, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pView->_count) return STRING_NF;\
    for (size_t i = 0; i <= pView->_count -querySize; i++)\
    {\
        if (_stringDirectCmp##genericSuffix(pView->_pBuffer +i, pQuery, querySize))\
            return i;\
    }\
    return STRING_NF;\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##FindNT(const viewName* pView, const Type* pQuery)\
{\
    return viewPrefix##FindPS(pView, pQuery, countNTSize##genericSuffix(pQuery));\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##FindS(const viewName* pView, const stringName* pQuery)\
{\
    return viewPrefix##FindPS(pView, pQuery->_pBuffer, pQuery->_count);\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##FindVw(const viewName* pView, const viewName* pQuery)\
{\
    return viewPrefix##FindPS(pView, pQuery->_pBuffer, pQuery->_count);\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##RevFindPS(const viewName* pView, const Type* pQuery, size_t querySize)\
{\
    if (querySize > pView->_count) return STRING_NF;\
    for (size_t i = pView->_count -querySize; i != (size_t)(-1); i--)\
    {\
        if (_stringDirectCmp##genericSuffix(pView->_pBuffer +i, pQuery, querySize))\
            return i;\
    }\
    return STRING_NF;\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##RevFindNT(const viewName* pView, const Type* pQuery)\
{\
    return viewPrefix##RevFindPS(pView, pQuery, countNTSize##genericSuffix(pQuery));\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##RevFindS(const viewName* pView, const stringName* pQuery)\
{\
    return viewPrefix##RevFindPS(pView, pQuery->_pBuffer, pQuery->_count);\
}\
static inline _mem_FUNC_ATTRIB size_t viewPrefix##RevFindVw(const viewName* pView, const viewName* pQuery)\
{\
    return viewPrefix##RevFindPS(pView, pQuery->_pBuffer, pQuery->_count);\
}



typedef char UTF8_t;
typedef char16_t UTF16_t;

STRING_DEF(String8, string8, View8, view8, 8, UTF8_t)
STRING_DEF(String16, string16, View16, view16, 16, UTF16_t)

DARRAY_DEF(Dstr, dstr, String8)
DARRAY_DEF(Dview, dview, View8)



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
static inline _mem_FUNC_ATTRIB bool isSpace16(const UTF16_t codeUnit)
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
static inline _mem_FUNC_ATTRIB void string16TrimStart(String16* pString)
{
    size_t count = 0;
    while (count < pString->_count && isSpace16(pString->_pBuffer[count]))
        count++;
    string16EraseStart(pString, count);
}
static inline _mem_FUNC_ATTRIB void view16TrimStart(View16* pView)
{
    size_t count = 0;
    while (count < pView->_count && isSpace16(pView->_pBuffer[count]))
        count++;
    pView->_pBuffer += count;
    pView->_count -= count;
}
static inline _mem_FUNC_ATTRIB void string16TrimEnd(String16* pString)
{
    size_t count = 0;
    while (count < pString->_count && isSpace16(pString->_pBuffer[pString->_count -count -1]))
        count++;
    string16EraseEnd(pString, count);
}
static inline _mem_FUNC_ATTRIB void view16TrimEnd(View16* pView)
{
    size_t count = 0;
    while (count < pView->_count && isSpace16(pView->_pBuffer[pView->_count -count -1]))
        count++;
    pView->_count -= count;
    if (count != 0) pView->_terminated = false;
}
static inline _mem_FUNC_ATTRIB void string16Trim(String16* pString)
{
    string16TrimStart(pString);
    string16TrimEnd(pString);
}
static inline _mem_FUNC_ATTRIB void view16Trim(View16* pView)
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
static inline _mem_FUNC_ATTRIB bool isSpacePS8(const UTF8_t* pCodeUnits, size_t availableCount, size_t* pSpaceSize_optional)
{
    const UTF8_t** pWhitespaceStringArray = _stringWhitespaceStrings8;
    const size_t whitespaceStringCount = sizeof(_stringWhitespaceStrings8) / sizeof(_stringWhitespaceStrings8[0]);

    /* Loop over all whitespace strings */
    for (size_t wsStrI = 0; wsStrI < (whitespaceStringCount); wsStrI++)
    {
        const size_t wsStrSize = countNTSizeLimited8(pWhitespaceStringArray[wsStrI], 3);
        /* Too little code units for comparison */
        if (availableCount < wsStrSize) continue;
        if (_stringDirectCmp8(pCodeUnits, pWhitespaceStringArray[wsStrI], wsStrSize))
        {
            if (pSpaceSize_optional != NULL) *pSpaceSize_optional = wsStrSize;
            return true;
        }
    }
    return false;
}
static inline _mem_FUNC_ATTRIB bool isSpaceNT8(const UTF8_t* pCodeUnits, size_t* pSpaceSize_optional)
{
    return isSpacePS8(pCodeUnits, countNTSizeLimited8(pCodeUnits, 3), pSpaceSize_optional);
}
static inline _mem_FUNC_ATTRIB bool revIsSpacePS8(const UTF8_t* pCodeUnits, size_t availableCount, size_t* pSpaceSize_optional)
{
    size_t skippedCUCount = 0;
    for (size_t srcI = availableCount -1; srcI != (size_t)(-1); srcI--, skippedCUCount++)
    {
        // Find the trailing code unit
        // Max space is 3 UTF-8 code units
        if (skippedCUCount == 3) break;
        if ((pCodeUnits[srcI +0] & 0b11000000) != 0b10000000)
        {
            // Check space
            size_t wsSize = 0;
            const size_t expectedSize = 1 +skippedCUCount;
            if (isSpacePS8(pCodeUnits +srcI, expectedSize, &wsSize)
            && wsSize == expectedSize)
            {
                if (pSpaceSize_optional != NULL) *pSpaceSize_optional = wsSize;
                return true;
            }
            else break;
        }
    }
    return false;
}
static inline _mem_FUNC_ATTRIB void view8TrimStart(View8* pView)
{
    size_t count = 0;
    size_t wsSize = 0; // Amount of code units in the found whitespace
    while (count < pView->_count
    && isSpacePS8(&pView->_pBuffer[count], pView->_count, &wsSize))
        count += wsSize;
    pView->_pBuffer += count;
    pView->_count -= count;
}
static inline _mem_FUNC_ATTRIB void string8TrimStart(String8* pString)
{
    size_t count = 0;
    size_t wsSize = 0; // Amount of code units in the found whitespace
    while (count < pString->_count
    && isSpacePS8(&pString->_pBuffer[count], pString->_count -count, &wsSize))
        count += wsSize;
    string8EraseStart(pString, count);
}
static inline _mem_FUNC_ATTRIB void view8TrimEnd(View8* pView)
{
    size_t count = 0;
    size_t wsSize = 0;
    while (count < pView->_count
    && revIsSpacePS8(&pView->_pBuffer[count], pView->_count -count, &wsSize))
        count += wsSize;
    pView->_count -= count;
    if (count != 0) pView->_terminated = false;
}
static inline _mem_FUNC_ATTRIB void string8TrimEnd(String8* pString)
{
    size_t count = 0;
    size_t wsSize = 0;
    while (count < pString->_count
    && revIsSpacePS8(pString->_pBuffer, pString->_count -count, &wsSize))
        count += wsSize;
    string8EraseEnd(pString, count);
}
static inline _mem_FUNC_ATTRIB void view8Trim(View8* pView)
{
    view8TrimStart(pView);
    view8TrimEnd(pView);
}
static inline _mem_FUNC_ATTRIB void string8Trim(String8* pString)
{
    string8TrimStart(pString);
    string8TrimEnd(pString);
}



// Returns true on fully valid input conversion
static inline _mem_FUNC_ATTRIB bool _stringUTF8to16(const UTF8_t* pSrc8, const size_t size8, String16* pDstString16)
{
    // Clear previous size
    string16Clear(pDstString16);
    // Initial result size would be AT LEAST, MINIMUM equal to UTF-8 code units
    // And string funcs would reallocate if required, so all is good
    string16Reserve(pDstString16, size8);

    bool foundBadCUs = false;
    for (size_t srcI = 0; srcI < size8; )
    {
        // One code unit
        if ((pSrc8[srcI +0] & 0b10000000) == 0b0)
        {
            string16AppendCU(pDstString16, (UTF16_t)pSrc8[srcI +0]);
            srcI += 1;
        }
        // Two code units
        else if ((pSrc8[srcI +0] & 0b11100000) == 0b11000000)
        {
            // Validate second code unit
            if ((srcI +1) >= size8
            || (pSrc8[srcI +1] & 0b11000000) != 0b10000000)
            {
                string16AppendCU(pDstString16, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            string16AppendCU(pDstString16,
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
                string16AppendCU(pDstString16, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            string16AppendCU(pDstString16,
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
                string16AppendCU(pDstString16, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
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
            string16AppendCU(pDstString16,
                (UTF16_t)(0xD800 + (conversionValue >> 10))
            );
            // Low surrogate
            string16AppendCU(pDstString16,
                (UTF16_t)(0xDC00 + (conversionValue & 0b1111111111))
            );
            srcI += 4;
        }
        // Invalid first code unit
        else
        {
            string16AppendCU(pDstString16, (UTF16_t)0xFFFD); // U+FFFD Replacement Character
            foundBadCUs = true;
            srcI += 1;
            continue;
        }
    }
    return !foundBadCUs;
}
// Returns true on fully valid input conversion
static inline _mem_FUNC_ATTRIB bool _stringUTF16to8(const UTF16_t* pSrc16, const size_t size16, String8* pDstString8)
{
    // Clear previous size
    string8Clear(pDstString8);
    // Initial result size would be AT LEAST, MINIMUM equal to UTF-16 code units
    // And string funcs would reallocate if required, so all is good
    string8Reserve(pDstString8, size16);

    bool foundBadCUs = false;
    for (size_t srcI = 0; srcI < size16; )
    {
        // High surrogate
        if (pSrc16[srcI +0] >= 0xD800 && pSrc16[srcI +0] <= 0xDBFF)
        {
            // Validate second code unit (must be a low surrogate)
            if (pSrc16[srcI +1] < 0xDC00 || pSrc16[srcI +1] > 0xDFFF)
            {
                string8AppendPS(pDstString8, (UTF8_t[]){0xEF, 0xBF, 0xBD}, 3); // U+FFFD Replacement Character
                foundBadCUs = true;
                srcI += 1;
                continue;
            }
            uint32_t conversionValue =
                (((uint32_t)(pSrc16[srcI +0] & 0b1111111111)) << 10) // High surrogate
                + (uint32_t)(pSrc16[srcI +1] & 0b1111111111) // Low surrogate
            + 0x10000U;
            string8AppendCU(pDstString8,
                (UTF8_t)(((conversionValue >> 18) & 0b00000111) | 0b11110000)
            );
            string8AppendCU(pDstString8,
                (UTF8_t)(((conversionValue >> 12) & 0b00111111) | 0b10000000)
            );
            string8AppendCU(pDstString8,
                (UTF8_t)(((conversionValue >> 6) & 0b00111111) | 0b10000000)
            );
            string8AppendCU(pDstString8,
                (UTF8_t)((conversionValue & 0b00111111) | 0b10000000)
            );
            srcI += 2;
        }
        // BMP code unit, not a high surrogate in a pair
        // Validate (musn't be a low surrogate, and is in the BMP)
        else if (pSrc16[srcI +0] < 0xDC00 || pSrc16[srcI +0] > 0xDFFF)
        {
            // Convert to one UTF-8 code unit
            if (pSrc16[srcI +0] <= 0x007F)
            {
                string8AppendCU(pDstString8, // ERROR HERE **********
                    (UTF8_t)pSrc16[srcI +0]
                );
            }
            // Convert to two UTF-8 code units
            else if (pSrc16[srcI +0] <= 0x07FF)
            {
                string8AppendCU(pDstString8,
                    (UTF8_t)(((pSrc16[srcI +0] >> 6) & 0b0011111) | 0b11000000)
                );
                string8AppendCU(pDstString8,
                    (UTF8_t)((pSrc16[srcI +0] & 0b0111111) | 0b10000000)
                );
            }
            // Convert to three UTF-8 code units
            else if (pSrc16[srcI +0] <= 0xFFFF)
            {
                string8AppendCU(pDstString8,
                    (UTF8_t)(((pSrc16[srcI +0] >> 12) & 0b00001111) | 0b11100000)
                );
                string8AppendCU(pDstString8,
                    (UTF8_t)(((pSrc16[srcI +0] >> 6) & 0b00111111) | 0b10000000)
                );
                string8AppendCU(pDstString8,
                    (UTF8_t)((pSrc16[srcI +0] & 0b00111111) | 0b10000000)
                );
            }
            srcI += 1;
        }
        // Invalid first code unit
        else
        {
            string8AppendPS(pDstString8, (UTF8_t[]){0xEF, 0xBF, 0xBD}, 3); // U+FFFD Replacement Character
            foundBadCUs = true;
            srcI += 1;
            continue;
        }
    }
    return !foundBadCUs;
}

static inline _mem_FUNC_ATTRIB bool string8CopyS16(String8* pDstString, const String16* pSrcString)
{
    return _stringUTF16to8(pSrcString->_pBuffer, pSrcString->_count, pDstString);
}
static inline _mem_FUNC_ATTRIB bool string8CopyVw16(String8* pDstString, const View16* pSrcView)
{
    return _stringUTF16to8(pSrcView->_pBuffer, pSrcView->_count, pDstString);
}
static inline _mem_FUNC_ATTRIB bool string8CopyNT16(String8* pDstString, const UTF16_t* pSrcText)
{
    return _stringUTF16to8(pSrcText, countNTSize16(pSrcText), pDstString);
}
static inline _mem_FUNC_ATTRIB bool string8CopyPS16(String8* pDstString, const UTF16_t* pSrcText, size_t size)
{
    return _stringUTF16to8(pSrcText, size, pDstString);
}

static inline _mem_FUNC_ATTRIB bool string16CopyS8(String16* pDstString, const String8* pSrcString)
{
    return _stringUTF8to16(pSrcString->_pBuffer, pSrcString->_count, pDstString);
}
static inline _mem_FUNC_ATTRIB bool string16CopyVw8(String16* pDstString, const View8* pSrcView)
{
    return _stringUTF8to16(pSrcView->_pBuffer, pSrcView->_count, pDstString);
}
static inline _mem_FUNC_ATTRIB bool string16CopyNT8(String16* pDstString, const UTF8_t* pSrcText)
{
    return _stringUTF8to16(pSrcText, countNTSize8(pSrcText), pDstString);
}
static inline _mem_FUNC_ATTRIB bool string16CopyPS8(String16* pDstString, const UTF8_t* pSrcText, size_t size)
{
    return _stringUTF8to16(pSrcText, size, pDstString);
}



// static inline _mem_FUNC_ATTRIB Dview _stringSplit8(const UTF8_t* pSrc, size_t srcSize, const UTF8_t* pDelimiter, size_t deliSize,
//                                         bool discardEmptyResults, bool contiguousAreOne)
// {
//     // Check args
//     if (srcSize == 0)
//     {
//         fprintf(stderr, "srcSize = 0 given to %s for pSrc %p\n",
//                         __func__, pSrc);
//         exit(EXIT_FAILURE);
//     }
//     else if (deliSize == 0)
//     {
//         fprintf(stderr, "deliSize = 0 given to %s for pDelimiter %p\n",
//                         __func__, pDelimiter);
//         exit(EXIT_FAILURE);
//     }
//     Dview result = {0};
//     size_t startI = 0;
//     for (size_t i = 0; i <= srcSize; )
//     {
//         bool firstCPIsDelimiter = false;
//         if (i +deliSize <= srcSize)
//             firstCPIsDelimiter = _stringDirectCmp8(pSrc +i, pDelimiter, deliSize);
//         if (firstCPIsDelimiter || i == srcSize)
//         {
//             if (i -startI != 0 || !discardEmptyResults)
//                 dviewAppendV(&result, view8MakeCopyPS(pSrc +startI, i -startI));
//             i += deliSize;
//             startI = i;
//             if (contiguousAreOne && firstCPIsDelimiter)
//             {
//                 while (i +deliSize <= srcSize)
//                 {
//                     if (!_stringDirectCmp8(pSrc +i, pDelimiter, deliSize)) break;
//                     else i += deliSize;
//                 }
//                 startI = i;
//             }
//         }
//         else i++;
//     }
//     return result;
// }

// static inline _mem_FUNC_ATTRIB Dview view8SplitPS(const View8* pView, const UTF8_t* pDelimiter, size_t size,
//                                     bool discardEmptyResults, bool contiguousAreOne)
// {
//     return _stringSplit8(pView->_pBuffer, pView->_count, pDelimiter, size,
//                         discardEmptyResults, contiguousAreOne);
// }
// static inline _mem_FUNC_ATTRIB Dview view8SplitNT(const View8* pView, const UTF8_t* pDelimiter,
//                                     bool discardEmptyResults, bool contiguousAreOne)
// {
//     return _stringSplit8(pView->_pBuffer, pView->_count, pDelimiter, countNTSize8(pDelimiter),
//                         discardEmptyResults, contiguousAreOne);
// }

// static inline _mem_FUNC_ATTRIB Dview string8SplitPS(const String8* pString, const UTF8_t* pDelimiter, size_t size,
//                                     bool discardEmptyResults, bool contiguousAreOne)
// {
//     return _stringSplit8(pString->_pBuffer, pString->_count, pDelimiter, size,
//                         discardEmptyResults, contiguousAreOne);
// }
// static inline _mem_FUNC_ATTRIB Dview string8SplitNT(const String8* pString, const UTF8_t* pDelimiter,
//                                     bool discardEmptyResults, bool contiguousAreOne)
// {
//     return _stringSplit8(pString->_pBuffer, pString->_count, pDelimiter, countNTSize8(pDelimiter),
//                         discardEmptyResults, contiguousAreOne);
// }
