#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "./darray.h"

void _darrayRealloc(_darrayBytes* pDarray, size_t newCapacity, size_t itemSize)
{
    // Check bad arguments
    if (pDarray == NULL)
    {
        fprintf(stderr, "pDarray = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }

    // Do nothing if same capacity
    if (newCapacity == pDarray->_itemsCapacity) return;
    // Free buffer if 0 size
    else if (newCapacity == 0)
    {
        free(pDarray->_pBytes);
        pDarray->_pBytes = NULL;
        pDarray->_itemsCapacity = 0;
        pDarray->_itemsCount = 0;
    }
    // Allocate buffer if null, or reallocate buffer
    else
    {
        void* pNewBuffer = realloc(pDarray->_pBytes, newCapacity *itemSize);
        if (pNewBuffer == NULL)
        {
            fprintf(stderr, "failed to reallocate darray %p from %zu to %zu\n",
                            pDarray, pDarray->_itemsCapacity, newCapacity);
            exit(EXIT_FAILURE);
        }
        pDarray->_pBytes = (byte_t*)pNewBuffer;
        pDarray->_itemsCapacity = newCapacity;
        if (pDarray->_itemsCount > newCapacity) pDarray->_itemsCount = newCapacity;
    }
}

void _darrayMakeFill(_darrayBytes* pDarray, size_t count, const void* pItem, size_t itemSize)
{
    // Check bad arguments
    if (pDarray == NULL)
    {
        fprintf(stderr, "pDarray = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    else if (pItem == NULL)
    {
        fprintf(stderr, "pItem = NULL given to %s for darray %p\n",
                        __func__, pDarray);
        exit(EXIT_FAILURE);
    }
    else if (count == 0)
    {
        fprintf(stderr, "count = 0 given to %s for darray %p\n",
                        __func__, pDarray);
        exit(EXIT_FAILURE);
    }

    // Allocate new buffer (or reallocate previous one)
    _darrayRealloc(pDarray, count, itemSize);

    // Fill buffer
    byte_t* const pBuffer = pDarray->_pBytes;
    const size_t totalByteCount = count *itemSize;
    size_t writtenByteCount = itemSize;
    // Fill one item instance first
    if (count > 0)
        memcpy(pBuffer, pItem, itemSize);
    // Repeat doubling the fill
    while (writtenByteCount *2 <= totalByteCount)
    {
        memcpy(pBuffer +writtenByteCount, pBuffer, writtenByteCount);
        writtenByteCount*= 2;
    }
    // Fill remaining bytes
    if (writtenByteCount < totalByteCount)
        memcpy(pBuffer +writtenByteCount, pBuffer, totalByteCount -writtenByteCount);

    // Set new count
    pDarray->_itemsCount = count;
}

void _darrayMakeCopy(_darrayBytes* pDarray, const void* pItems, size_t itemSize, size_t itemCount)
{
    // Check bad arguments
    if (pDarray == NULL)
    {
        fprintf(stderr, "pDarray = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    else if (pItems == NULL)
    {
        fprintf(stderr, "pItems = NULL given to %s for darray %p\n",
                        __func__, pDarray);
        exit(EXIT_FAILURE);
    }

    // Allocate new buffer (or reallocate previous one)
    _darrayRealloc(pDarray, itemCount, itemSize);
    // Copy into buffer
    memcpy(pDarray->_pBytes, pItems, itemCount *itemSize);
    // Set new count
    pDarray->_itemsCount = itemCount;
}

void _darrayInsert(_darrayBytes* pDarray, size_t index, const void* pItems, size_t itemSize, size_t itemCount)
{
    // Check bad arguments
    if (pDarray == NULL)
    {
        fprintf(stderr, "pDarray = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    else if (pItems == NULL)
    {
        fprintf(stderr, "pItems = NULL given to %s for darray %p\n",
                        __func__, pDarray);
        exit(EXIT_FAILURE);
    }
    else if (itemCount == 0)
    {
        fprintf(stderr, "itemCount = 0 given to %s for darray %p\n",
                        __func__, pDarray);
        exit(EXIT_FAILURE);
    }
    else if (index > pDarray->_itemsCount)
    {
        fprintf(stderr, "index = %zu > pDarray->_itemsCount given to %s for darray %p\n",
                        index, __func__, pDarray);
        exit(EXIT_FAILURE);
    }

    // Reallocate buffer with more capacity if not enough (or allocate new one)
    if (pDarray->_itemsCapacity < pDarray->_itemsCount +itemCount)
        _darrayRealloc(pDarray, (pDarray->_itemsCount +itemCount) *2, itemSize);
    // Make space for insertion
    memmove(pDarray->_pBytes + ((index +itemCount) *itemSize),
            pDarray->_pBytes + (index *itemSize),
            (pDarray->_itemsCount -index) *itemSize);
    // Copy new items into buffer
    memcpy(pDarray->_pBytes +(index *itemSize), pItems, itemCount *itemSize);
    // Set new count
    pDarray->_itemsCount += itemCount;
}

void _darrayErase(_darrayBytes* pDarray, size_t startI, size_t count, size_t itemSize)
{
    // Check bad arguments
    if (pDarray == NULL)
    {
        fprintf(stderr, "pDarray = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    else if (startI >= pDarray->_itemsCount)
    {
        fprintf(stderr, "startI = %zu >= pDarray->_itemsCount given to %s for darray %p\n",
                        startI, __func__, pDarray);
        exit(EXIT_FAILURE);
    }
    else if (count > pDarray->_itemsCount -startI)
    {
        fprintf(stderr, "count = %zu > (pDarray->_itemsCount -startI) given to %s for darray %p\n",
                        count, __func__, pDarray);
        exit(EXIT_FAILURE);
    }

    // Move back the front side of the buffer to erase whats before it
    size_t shiftedCount = pDarray->_itemsCount -(startI +count);
    memmove(pDarray->_pBytes +(startI *itemSize), 
            pDarray->_pBytes +((startI +count) *itemSize), 
            shiftedCount *itemSize);
    pDarray->_itemsCount -= count;
}

void *_darrayAt(const _darrayBytes* pDarray, size_t index, size_t itemSize)
{
    // Check bad arguments
    if (pDarray == NULL)
    {
        fprintf(stderr, "pDarray = NULL given to %s\n",
                        __func__);
        exit(EXIT_FAILURE);
    }
    else if (index >= pDarray->_itemsCount)
    {
        fprintf(stderr, "index = %zu >= pDarray->_itemsCount given to %s for darray %p\n",
                        index, __func__, pDarray);
        exit(EXIT_FAILURE);
    }

    return &(pDarray->_pBytes[index *itemSize]);
}
