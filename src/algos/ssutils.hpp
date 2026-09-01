#pragma once
#include <utility>
#include <string>
#include "utils.hpp"
// System-specific utils
namespace SSUtils {

std::pair<std::string, Utils::ErrorState> utf16to8(const std::u16string_view inputView);
std::pair<std::u16string, Utils::ErrorState> utf8to16(const std::string_view inputView);

/*
enum class PathType : uint8_t
{
    None, Disk, Dir, File
};

// Returns path type and proper converted path if the situation requires
std::pair<PathType, std::string> getPathInfo(std::string_view pathView, PathType expectedType);
*/

}
