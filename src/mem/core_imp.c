#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "./core.h"

static void _memRepeatMemcpy(byte_t* pBytes, size_t count, const void* pItem, size_t itemSize)
{
    // Check bad arguments
    if (pItem == NULL)
    {
        fprintf(stderr, "pItem = NULL given to %s for buffer %p\n",
                        __func__, pBytes);
        exit(EXIT_FAILURE);
    }
    else if (count == 0)
    {
        fprintf(stderr, "count = 0 given to %s for buffer %p\n",
                        __func__, pBytes);
        exit(EXIT_FAILURE);
    }

    // Fill buffer
    const size_t goalBytes = count *itemSize;
    size_t writtenBytes = 0;
    // Fill one item instance first
    if (count > 0)
    {
        memcpy(pBytes, pItem, itemSize);
        writtenBytes = 1 *itemSize;
    }
    // Repeat doubling the fill
    while (writtenBytes *2 <= goalBytes)
    {
        memcpy(pBytes +writtenBytes, pBytes, writtenBytes);
        writtenBytes*= 2;
    }
    // Fill remaining bytes
    if (writtenBytes < goalBytes)
        memcpy(pBytes +writtenBytes, pBytes, goalBytes -writtenBytes);
}

void _memRealloc(_memGS* pGS, size_t newCapacity, size_t itemSize)
{
    // Do nothing if same capacity
    if (newCapacity == pGS->_itemsCapacity) return;
    // Free buffer if 0 size
    else if (newCapacity == 0)
    {
        free(pGS->_pBytes);
        pGS->_pBytes = NULL;
        pGS->_itemsCount = 0;
        pGS->_itemsCapacity = 0;
    }
    // Allocate buffer if null, or reallocate buffer
    else
    {
        byte_t* pNewBuffer = (byte_t*)realloc(pGS->_pBytes, newCapacity *itemSize);
        if (pNewBuffer == NULL)
        {
            fprintf(stderr, "failed to reallocate GS %p from %zu to %zu in %s\n",
                            pGS, pGS->_itemsCapacity, newCapacity, __func__);
            exit(EXIT_FAILURE);
        }
        pGS->_pBytes = pNewBuffer;
        if (pGS->_itemsCount > newCapacity) pGS->_itemsCount = newCapacity;
        pGS->_itemsCapacity = newCapacity;
    }
}
void _memResize(_memGS* pGS, size_t newSize, const void* pItem, size_t itemSize)
{
    size_t ogCount = pGS->_itemsCount;\
    if (newSize > ogCount)\
    {\
        if (newSize > pGS->_itemsCapacity)\
            _memRealloc(pGS, newSize, itemSize);\
        /* Fill new area with given value */\
        /* Using _memRepeatMemcpy() for fast memcpy algorithm*/\
        _memRepeatMemcpy(pGS->_pBytes +(ogCount *itemSize),\
                        newSize -ogCount, pItem, itemSize);\
    }\
    pGS->_itemsCount = newSize;\
}

void _memFill(_memGS* pGS, size_t count, const void* pItem, size_t itemSize)
{
    // Check bad arguments
    if (pItem == NULL)
    {
        fprintf(stderr, "pItem = NULL given to %s for GS %p\n",
                        __func__, pGS);
        exit(EXIT_FAILURE);
    }
    else if (count == 0)
    {
        fprintf(stderr, "count = 0 given to %s for GS %p\n",
                        __func__, pGS);
        exit(EXIT_FAILURE);
    }

    // Allocate new buffer (or reallocate previous one)
    _memRealloc(pGS, count, itemSize);
    // Fill buffer
    _memRepeatMemcpy(pGS->_pBytes, count, pItem, itemSize);
    // Set new count
    pGS->_itemsCount = count;
}
void _memCopy(_memGS* pGS, const void* pItems, size_t itemSize, size_t itemCount)
{
    // Check bad arguments
    if (pItems == NULL)
    {
        fprintf(stderr, "pItems = NULL given to %s for GS %p\n",
                        __func__, pGS);
        exit(EXIT_FAILURE);
    }

    // Allocate new buffer (or reallocate previous one)
    _memRealloc(pGS, itemCount, itemSize);
    // Copy into buffer
    memcpy(pGS->_pBytes, pItems, itemCount *itemSize);
    // Set new count
    pGS->_itemsCount = itemCount;
}

void _memInsert(_memGS* pGS, size_t index, const void* pItems, size_t itemSize, size_t itemCount)
{
    // Check bad arguments
    if (pItems == NULL)
    {
        fprintf(stderr, "pItems = NULL given to %s for GS %p\n",
                        __func__, pGS);
        exit(EXIT_FAILURE);
    }
    else if (itemCount == 0)
    {
        fprintf(stderr, "itemCount = 0 given to %s for GS %p\n",
                        __func__, pGS);
        exit(EXIT_FAILURE);
    }
    else if (index > pGS->_itemsCount)
    {
        fprintf(stderr, "index = %zu > pGS->_itemsCount given to %s for GS %p\n",
                        index, __func__, pGS);
        exit(EXIT_FAILURE);
    }

    // Reallocate buffer with more capacity if not enough (or allocate new one)
    if (pGS->_itemsCapacity < pGS->_itemsCount +itemCount)
        _memRealloc(pGS, (pGS->_itemsCount +itemCount) *2, itemSize);
    // Shift data forwards to make space for insertion
    memmove(pGS->_pBytes + ((index +itemCount) *itemSize),
            pGS->_pBytes + (index *itemSize),
            (pGS->_itemsCount -index) *itemSize);
    // Copy new items into buffer
    memcpy(pGS->_pBytes +(index *itemSize), pItems, itemCount *itemSize);
    // Set new count
    pGS->_itemsCount += itemCount;
}
void _memAppendNull(_memGS* pGS, const void* pItem, size_t itemSize)
{
    // Reallocate buffer with more capacity if not enough (or allocate new one)
    if (pGS->_itemsCapacity < pGS->_itemsCount +1)
        _memRealloc(pGS, (pGS->_itemsCount +1) *2, itemSize);
    
    // Copy null at end of buffer
    memcpy(pGS->_pBytes +(pGS->_itemsCount *itemSize), pItem, itemSize);
    // Set new count
    pGS->_itemsCount += 1;
}
void _memErase(_memGS* pGS, size_t startI, size_t count, size_t itemSize)
{
    // Check bad arguments
    if (startI +count > pGS->_itemsCount)
    {
        fprintf(stderr, "(startI +count) = %zu > pGS->_itemsCount given to %s for GS %p\n",
                        count, __func__, pGS);
        exit(EXIT_FAILURE);
    }
    
    // Move back the front side of the buffer to erase whats before it
    size_t shiftedCount = pGS->_itemsCount -(startI +count);
    memmove(pGS->_pBytes +(startI *itemSize), 
            pGS->_pBytes +((startI +count) *itemSize), 
            shiftedCount *itemSize);
    pGS->_itemsCount -= count;
}

void* _memAt(const _memGS* pGS, size_t index, size_t itemSize)
{
    // Check bad arguments
    if (index >= pGS->_itemsCount)
    {
        fprintf(stderr, "index = %zu >= pGS->_itemsCount given to %s for GS %p\n",
                        index, __func__, pGS);
        exit(EXIT_FAILURE);
    }

    // JIC compiler is angry at const -> mutable
    return (void*) (
        (uintptr_t) (
            & (pGS->_pBytes[index *itemSize])
        )
    );
}



static void _memStrInsertNull(_memGS* pGS, size_t itemSize, const void* pNull)
{
    // Set null terminator if capacity allows
    if (pGS->_itemsCapacity > pGS->_itemsCount)
        memcpy(&pGS->_pBytes[pGS->_itemsCount *itemSize], pNull, itemSize);
    // Reallocate and set null terminator
    else
    {
        _memInsert(pGS, pGS->_itemsCount, pNull, itemSize, 1);
        pGS->_itemsCount--;
    }
}

void _memStrRealloc(_memGS* pGS, size_t newCapacity, size_t itemSize,
                        const void* pNull)
{
    _memRealloc(pGS, newCapacity, itemSize);
    _memStrInsertNull(pGS, itemSize, pNull);
}
void _memStrResize(_memGS* pGS, size_t newSize, const void* pItem, size_t itemSize,
                        const void* pNull)
{
    _memResize(pGS, newSize, pItem, itemSize);
    _memStrInsertNull(pGS, itemSize, pNull);
}

void _memStrFill(_memGS* pGS, size_t count, const void* pItem, size_t itemSize,
                        const void* pNull)
{
    _memFill(pGS, count, pItem, itemSize);
    _memStrInsertNull(pGS, itemSize, pNull);
}
void _memStrCopy(_memGS* pGS, const void* pItems, size_t itemSize, size_t itemCount,
                        const void* pNull)
{
    _memCopy(pGS, pItems, itemSize, itemCount);
    _memStrInsertNull(pGS, itemSize, pNull);
}

void _memStrInsert(_memGS* pGS, size_t index, const void* pItems, size_t itemSize, size_t itemCount,
                        const void* pNull)
{
    _memInsert(pGS, index, pItems, itemSize, itemCount);
    _memStrInsertNull(pGS, itemSize, pNull);
}
void _memStrErase(_memGS* pGS, size_t startI, size_t count, size_t itemSize,
                        const void* pNull)
{
    _memErase(pGS, startI, count, itemSize);
    _memStrInsertNull(pGS, itemSize, pNull);
}
