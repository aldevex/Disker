#pragma once
#include "./core.h"

// Open File Method
typedef enum base_OFMethod
{
    BASE_OFMETHOD_NULL,
    BASE_OFMETHOD_OPEN_EXISTING, // Open an existing file
    BASE_OFMETHOD_CREATE_ALWAYS, // Create a new file in all cases
    BASE_OFMETHOD_CREATE_CONDITIONALLY // Open the file if it exists, create otherwise
} base_OFMethod;
// Also locks the file and sets initial file pointer to 0
handle_t base_openFile(const UTF8_t* pPath, base_OFMethod method, bool* pCreatedNewFileOptional);
// Also sets handle to HANDLE_NONE (even on failure)
es_t base_closeFile(handle_t* pHandle);

es_t base_seekFile(handle_t handle, uint64_t offset);
es_t base_readFile(handle_t handle, uint64_t size, void* pBlock);
es_t base_writeFile(handle_t handle, uint64_t size, const void* pBlock);

es_t base_getFileInfo(handle_t handle, uint64_t* pSize, bool* pSparseOptional);
es_t base_resizeFile(handle_t handle, uint64_t newSize);
es_t base_setFileSparsity(handle_t handle, bool sparsity);



typedef enum base_PathType
{
    BASE_PATHTYPE_NULL, BASE_PATHTYPE_INVALID,
    BASE_PATHTYPE_FILE, BASE_PATHTYPE_DIR,
    BASE_PATHTYPE_CREATABLE_FILE,
    BASE_PATHTYPE_VOLUME, BASE_PATHTYPE_DISK
} base_PathType;

base_PathType base_getPathType(const UTF8_t* path, size_t pathSize);
