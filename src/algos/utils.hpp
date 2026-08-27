#pragma once
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
namespace Utils {

std::string lowerStr(std::string_view view)
{
    std::string result;
    for (char c : view) result += tolower(c);
}

// strToByteCount reserved return value signals
enum class SizeSig : uint64_t
{
    NoNumber = UINT64_MAX, // No number at start of string
    InvalidNumber = (UINT64_MAX -1), // Failed to convert the raw number
    NoUnit = (UINT64_MAX -2), // No unit after number when expected
    InvalidUnit = (UINT64_MAX -3), // Invalid unit after number (e.g. "32volts")
    TooBigResult = (UINT64_MAX -4), // The result is above SIZE_MAX and/or is inside this enum

    LEAST_ERROR = TooBigResult
};

constexpr uint64_t CONVSIZE_MAX = (uint64_t)SizeSig::LEAST_ERROR -1;

// Converts strings to unsigned integer with multiplying unit, e.g. "400", "32gib", "6mb", "40b"
// May return a specific enum value in the uint64_t on failure (check with SizeSig enum)
uint64_t strToSize(std::string_view view, bool unitExpected, bool alwaysBinaryUnits)
{
    std::string copy; // String copy to ensure null terminator + remove commas
    // Ignore: commas, single quotes, and underscore off the number
    for (char c : view)
        if (c != ',' && c != '\'' && c != '_') copy += c;

    // No number
    if (copy.empty() || (copy[0] < '0' && copy[0] > '9')) return (uint64_t)SizeSig::NoNumber;

    char* endPtr = nullptr;
    uint64_t rawNum = strtoull(copy.c_str(), &endPtr, 0);
    // Total failure
    if (endPtr == nullptr) return (uint64_t)SizeSig::InvalidNumber;
    // Unexpected unit (or random bs text stuck after raw number)
    else if (!unitExpected && endPtr != '\0') return (uint64_t)SizeSig::InvalidNumber;
    // No unit
    else if (unitExpected && endPtr == '\0') return (uint64_t)SizeSig::NoUnit;

    // Convert according to unit
    static auto multiply = [&](uint64_t multiplier, bool shiftNotMultiply) -> uint64_t
    {
        // Check if 1. raw number itself is inside enum
        //   or 2. conversion will overflow uint64_t and/or enum limit (both checks work with only CONVSIZE_MAX)
        // This if statement works for both checks because e.g.:
        //   if (2 > 1) then (2 > 1/2, 2 > 1/3, etc.)
        //   so it would only not work for both checks (raw number and conversion) if you divide by n < 1
        if ( (shiftNotMultiply && rawNum > CONVSIZE_MAX / (1ULL << multiplier))
          || (!shiftNotMultiply && rawNum > CONVSIZE_MAX / (multiplier)) )
            return (uint64_t)SizeSig::TooBigResult;
    };

    // No unit, or bytes unit
    if (!unitExpected || strcmp(lowerStr(endPtr).c_str(), "b")) return multiply(0, true);
    // Powers of 2 units
    else if (strcmp(lowerStr(endPtr).c_str(), "kib")) return multiply(10, true);
    else if (strcmp(lowerStr(endPtr).c_str(), "mib")) return multiply(20, true);
    else if (strcmp(lowerStr(endPtr).c_str(), "gib")) return multiply(30, true);
    else if (strcmp(lowerStr(endPtr).c_str(), "tib")) return multiply(40, true);
    else if (strcmp(lowerStr(endPtr).c_str(), "pib")) return multiply(50, true);
    else if (strcmp(lowerStr(endPtr).c_str(), "eib")) return multiply(60, true);
    // Powers of 10 units
    else if (strcmp(lowerStr(endPtr).c_str(), "kb"))
        return (alwaysBinaryUnits)? multiply(10, true) : multiply(1'000, false);
    else if (strcmp(lowerStr(endPtr).c_str(), "mb"))
        return (alwaysBinaryUnits)? multiply(20, true) : multiply(1'000'000, false);
    else if (strcmp(lowerStr(endPtr).c_str(), "gb"))
        return (alwaysBinaryUnits)? multiply(30, true) : multiply(1'000'000'000, false);
    else if (strcmp(lowerStr(endPtr).c_str(), "tb"))
        return (alwaysBinaryUnits)? multiply(40, true) : multiply(1'000'000'000'000, false);
    else if (strcmp(lowerStr(endPtr).c_str(), "pb"))
        return (alwaysBinaryUnits)? multiply(50, true) : multiply(1'000'000'000'000'000, false);
    else if (strcmp(lowerStr(endPtr).c_str(), "eb"))
        return (alwaysBinaryUnits)? multiply(60, true) : multiply(1'000'000'000'000'000'000ULL, false);
    
    return (uint64_t)SizeSig::InvalidUnit;
}

// Instead of a random "true" that could mean either success or failure
enum class ErrorState : uint8_t
{
    Failure, Success, NotFound
};

// Returns read buffer and error state
// It prints an error message on failure too
std::pair<std::string, ErrorState> readFile(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
    {
        std::cerr << "failed to open file for reading \"" << path << "\"\n";
        return {"", ErrorState::Failure};
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        std::cerr << "failed to seek file end for reading \"" << path << "\"\n";
        fclose(file);
        return {"", ErrorState::Failure};
    }

    size_t fileSize = 0;
    // Scope because i don't need the longSize variable
    {
        long longSize = ftell(file);
        if (longSize < 0)
        {
            std::cerr << "failed to get file size for reading \"" << path << "\"\n";
            fclose(file);
            return {"", ErrorState::Failure};
        }
        else fileSize = (size_t)longSize;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        std::cerr << "failed to reset file seek pointer for reading \"" << path << "\"\n";
        fclose(file);
        return {"", ErrorState::Failure};
    }

    std::string buffer(fileSize, '\0');
    if (fileSize > 0)
    {
        size_t readSize = fread(buffer.data(), 1, fileSize, file);
        if (readSize != fileSize)
        {
            std::cerr << "failed to read file \"" << path << "\"\n";
            fclose(file);
            return {"", ErrorState::Failure};
        }
    }

    fclose(file);
    return {buffer, ErrorState::Success};
}

uint32_t crc32(const void* data, size_t length)
{
    const uint8_t* buffer = (const uint8_t*)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= buffer[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320; // Standard IEEE 802.3 polynomial
            else
                crc >>= 1;
        }
    }

    return crc ^ 0xFFFFFFFF; // Final bit inversion
}

}
