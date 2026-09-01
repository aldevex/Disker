#include <clocale>
#include <sys/stat.h>
#include <iostream>
#include "../algos/utils.hpp"
#include "../algos/ssutils.hpp"

namespace SSUtils
{
    /*
    // Returns:
    // NotFound = non-existent or non-file path
    // Failure = not a disk
    // Success = real disk
    Utils::ErrorState isDisk(const char* path)
    {
        struct stat st;
        if (stat(path, &st) != 0) return;

        if (S_ISREG(st.st_mode))
        {
            std::cout << "It's a regular file." << std::endl;
        } else if (S_ISBLK(st.st_mode))
        {
            std::cout << "It's a block device (physical disk/partition)." << std::endl;
        } else if (S_ISDIR(st.st_mode))
        {
            std::cout << "It's a directory." << std::endl;
        } else {
            std::cout << "It's something else (char device, socket, etc.)." << std::endl;
        }
    }
    */
} // namespace SSUtils
