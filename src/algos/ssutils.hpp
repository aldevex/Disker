#pragma once
#include <utility>
#include <string>
#include "utils.hpp"
// System-specific utils
namespace Utils {

std::pair<std::string, Utils::ErrorState> utf16to8(const std::u16string_view inputView);
std::pair<std::u16string, Utils::ErrorState> utf8to16(const std::string_view inputView);

std::pair<uint64_t, ErrorState> getFileSize(const char* path);

/*
enum class PathType : uint8_t
{
    None, File, Dir, DiskVolume, DiskDirect
};

std::pair<PathType, ErrorState> getPathType(std::string_view pathView);
*/

}
