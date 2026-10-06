#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

void embed(const char* srcPath, const char* dstPath, const char* varName);

int main()
{
    embed("build/diskerdev/mbr_default.bin",
            "src/disk/scheme/mbr_default.h",
            "_mbr_DATA_DEFAULT_BYTES");

    return EXIT_SUCCESS;
}

void embed(const char* srcPath, const char* dstPath, const char* varName)
{
    // Open source
    FILE* fp = fopen(srcPath, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "failed to open file %s\n", srcPath);
        exit(EXIT_FAILURE);
    }
    // Seek source end
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        fprintf(stderr, "failed to seek file end %s\n", srcPath);
        exit(EXIT_FAILURE);
    }
    // Get source size
    long signedSrcSize = ftell(fp);
    if (signedSrcSize < 0)
    {
        fclose(fp);
        fprintf(stderr, "failed to get file size %s\n", srcPath);
        exit(EXIT_FAILURE);
    }
    size_t srcSize = (size_t)signedSrcSize;
    // Reset seek index
    rewind(fp);
    // Allocate buffer
    uint8_t* buffer = (uint8_t*)malloc(srcSize);
    if (buffer == NULL)
    {
        fclose(fp);
        fprintf(stderr, "failed to allocate memory for %s\n", srcPath);
        exit(EXIT_FAILURE);
    }
    // Read file
    if (fread(buffer, 1, srcSize, fp) != srcSize)
    {
        free(buffer);
        fclose(fp);
        fprintf(stderr, "failed to read file %s\n", srcPath);
        exit(EXIT_FAILURE);
    }
    // Close file
    if (fclose(fp) != 0)
    {
        free(buffer);
        fclose(fp);
        fprintf(stderr, "failed to close file %s\n", srcPath);
        exit(EXIT_FAILURE);
    }
    
    // Open dst file
    fp = fopen(dstPath, "wb");
    if (fp == NULL)
    {
        free(buffer);
        fprintf(stderr, "failed to open file %s\n", dstPath);
        exit(EXIT_FAILURE);
    }
    // Reset seek index
    rewind(fp);
    // Write header
    const char* header = "//\n"
                        "// *** AUTOMATICALLY GENERATED FILE ***\n"
                        "//\n"
                        "#pragma once\n"
                        "#include \"../../mem/mem.h\"\n"
                        "\n"
                        "static const byte_t ";
    bool done = (fwrite(header, 1, strlen(header), fp) == strlen(header));
    // Write variable name
    if (done)
        done = (fwrite(varName, 1, strlen(varName), fp) == strlen(varName));
    // Write middle
    const char* middle = "[] = {"; // NO NEWLINE! LOOP WILL PRINT
    if (done)
        done = (fwrite(middle, 1, strlen(middle), fp) == strlen(middle));
    // Write bytes
    size_t writtenInline = 0;
    for (size_t i = 0; i < srcSize && done; i++, writtenInline++)
    {
        if (writtenInline == 7) writtenInline = 0;
        // Spacing
        if (writtenInline == 0) fprintf(fp, "\n    ");
        else fprintf(fp, " ");
        // Print
        done = (fprintf(fp, "0x%02x,", buffer[i]) >= 0);
    }
    // Write footer
    const char* footer = "\n};\n"; // Two newlines here ************
    if (done)
        done = (fwrite(footer, 1, strlen(footer), fp) == strlen(footer));
    // Write error
    if (!done)
    {
        free(buffer);
        fclose(fp);
        fprintf(stderr, "failed to write file %s\n", dstPath);
        exit(EXIT_FAILURE);
    }
    // Reset seek index
    rewind(fp);
    // Close file
    if (fclose(fp) != 0)
    {
        free(buffer);
        fclose(fp);
        fprintf(stderr, "failed to close file %s\n", dstPath);
        exit(EXIT_FAILURE);
    }
}
