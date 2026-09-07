#pragma once
#include <utility>
#include <string>
#include "utils.hpp"
#include "../structs/disk.hpp"
#include "../structs/mbr.hpp"
// System-specific utils
namespace Utils {

std::pair<std::string, Utils::ErrorState> utf16to8(const std::u16string_view inputView);
std::pair<std::u16string, Utils::ErrorState> utf8to16(const std::string_view inputView);

std::pair<uint64_t, ErrorState> getFileSize(const char* path);

enum class PathType : uint8_t
{
    Invalid, None, // Inexistent path
    File, Dir, DiskDirect,
    //DiskVolume
};

PathType getPathType(std::string_view pathView);

struct ReadImageInfo
{
    Disk::Disk disk = Disk::Disk();
    Disk::Scheme scheme = Disk::Scheme::MBR;
    MBR::MBRData mbrData = MBR::MBRData();
    //GPT::Header gptHeader;
    //std::vector<GPT::Entry> gptEntryArray;
    Utils::ErrorState errorState = Utils::ErrorState::Failure;

    ReadImageInfo() = default;
};

// Also prints errors
ReadImageInfo readImage(const char* path);

struct WriteImageInfo
{
    const uint8_t* pBuffer;
    size_t bufferSize;
    size_t fileWriteOffset;
    WriteImageInfo(const uint8_t* pBuffer, size_t bufferSize, size_t fileWriteOffset)
        : pBuffer(pBuffer), bufferSize(bufferSize), fileWriteOffset(fileWriteOffset)
    {}
};

// Accelerated image writing with system API (sparse file, or worst case scenario quick zeroing)
// Prints errors
Utils::ErrorState writeImage(const char* path, uint64_t fileSize,
                            const std::vector<WriteImageInfo>& writeInfos);

}
