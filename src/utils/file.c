#include "./file.h"

utils_ErrorState utils_readFile(const View8* pPathView, Dbyte* pDarray)
{
    // Check null termination and get C string
    if (!view8Terminated(pPathView))
    {
        fprintf(stderr, "unterminated path view given to \"%s\"\n", __func__);
        exit(EXIT_FAILURE);
    }
    const UTF8_t* path = view8Data(pPathView);

    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "failed to open file for reading \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fprintf(stderr, "failed to seek file end for reading \"%s\"\n", path);
        fclose(file);
        return UTILS_ERRORSTATE_FAILURE;
    }

    size_t fileSize = 0;
    // Scope because i don't need the longSize variable
    {
        long longSize = ftell(file);
        if (longSize < 0)
        {
            fprintf(stderr, "failed to get file size for reading \"%s\"\n", path);
            fclose(file);
            return UTILS_ERRORSTATE_FAILURE;
        }
        else fileSize = (size_t)longSize;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fprintf(stderr, "failed to reset file seek pointer for reading \"%s\"\n", path);
        fclose(file);
        return UTILS_ERRORSTATE_FAILURE;
    }

    Dbyte buffer;
    if (fileSize > 0)
    {
        dbyteResizeZ(&buffer, fileSize);
        size_t readSize = fread(dbyteData(&buffer), 1, fileSize, file);
        if (readSize != fileSize)
        {
            fprintf(stderr, "failed to read file \"%s\"\n", path);
            fclose(file);
            return UTILS_ERRORSTATE_FAILURE;
        }
    }

    if (fclose(file) != 0)
    {
        fprintf(stderr, "failed to close file after reading \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }
    *pDarray = buffer;
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState utils_writeFile(const View8* pPathView, const uint8_t* pBuffer, size_t bufferSize)
{
    // Check null termination and get C string
    if (!view8Terminated(pPathView))
    {
        fprintf(stderr, "unterminated path view given to \"%s\"\n", __func__);
        exit(EXIT_FAILURE);
    }
    const UTF8_t* path = view8Data(pPathView);

    FILE* file = fopen(path, "wb");
    if (file == NULL)
    {
        fprintf(stderr, "failed to open file for writing \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }

    if (bufferSize != 0)
    {
        size_t writtenSize = fwrite(pBuffer, 1, bufferSize, file);
        if (writtenSize != bufferSize)
        {
            fprintf(stderr, "failed to write file \"%s\"\n", path);
            fclose(file);
            return UTILS_ERRORSTATE_FAILURE;
        }
    }

    if (fclose(file) != 0)
    {
        fprintf(stderr, "failed to close file after writing \"%s\"\n", path);
        return UTILS_ERRORSTATE_FAILURE;
    }
    return UTILS_ERRORSTATE_SUCCESS;
}

// Implemented in system specific implementation files
// utils_ErrorState utils_getFileSize(const View8* pPathView, uint64_t* pSize);
