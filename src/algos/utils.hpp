#pragma once
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <span>
namespace Utils {

inline void printHelp()
{
    std::cout <<
        "Help string idfk what to put i will add later\n"
        ""
        ""
        ""
    << std::endl;
}

inline std::string lowerStr(std::string_view view)
{
    std::string result;
    for (char c : view) result += tolower(c);
    return result;
}

// Converts the first string (internally) to lower case then compares it to the second string
// Returns true if equal
inline bool compareLow(std::string_view anyCaseAnyLenView, std::string_view lowerCaseRequiredView)
{
    return (
        (anyCaseAnyLenView.length() == lowerCaseRequiredView.length())
        && !strncmp(Utils::lowerStr(anyCaseAnyLenView).c_str(), lowerCaseRequiredView.data(),
                    lowerCaseRequiredView.length())
    );
};

// strToByteCount reserved return value error signals
enum class SizeSig : uint64_t
{
    NoNumber = UINT64_MAX, // No number (empty string, or commas and other separators without a number)
    InvalidNumber = (UINT64_MAX -1), // Failed to convert the raw number
    NoUnit = (UINT64_MAX -2), // No unit after number when expected
    UnacceptableZero = (UINT64_MAX -3), // The number is zero when zero is unacceptable
    TooBigResult = (UINT64_MAX -4), // The result is above UINT64_MAX and/or is inside this enum
    InvalidUnit = (UINT64_MAX -5), // Invalid unit after number (e.g. "32volts")

    LEAST_ERROR = InvalidUnit // Smallest enum value
};

constexpr uint64_t CONVSIZE_MAX = (uint64_t)SizeSig::LEAST_ERROR -1;

// Converts strings to unsigned integer with multiplying unit, e.g. "400", "32gIB", "6Mb", "40B"
// May return a specific enum value in the uint64_t on failure (check with SizeSig enum)
// Does NOT print error messages on failure
// Easy way to remember parameter order: "0 then 2 (binary) then unit"
inline uint64_t strToSize(std::string_view view, bool zeroIsUnacceptable, bool alwaysBinaryUnits, bool unitExpected)
{
    std::string copy; // String copy to ensure null terminator + remove commas
    // Ignore: commas, single quotes, and underscore off the number
    for (char c : view)
        if (c != ',' && c != '\'' && c != '_') copy += c;

    // No number
    if (copy.empty()) return (uint64_t)SizeSig::NoNumber;

    char* endPtr = nullptr;
    uint64_t rawNum = strtoull(copy.c_str(), &endPtr, 0);
    // Total failure
    if (endPtr == nullptr) return (uint64_t)SizeSig::InvalidNumber;
    // Unexpected unit (or random bs text stuck after raw number)
    else if (!unitExpected && endPtr[0] != '\0') return (uint64_t)SizeSig::InvalidNumber;
    // No unit
    else if (unitExpected && endPtr[0] == '\0') return (uint64_t)SizeSig::NoUnit;
    // Unacceptable zero
    else if (zeroIsUnacceptable && rawNum == 0) return (uint64_t)SizeSig::UnacceptableZero;

    // Convert according to unit
    auto multiply = [&](uint64_t multiplier, bool shiftNotMultiply) -> uint64_t
    {
        // Check if 1. raw number itself is inside enum
        //   or 2. conversion will overflow uint64_t and/or enum limit (both checks work with only CONVSIZE_MAX)
        // This if statement works for both checks because e.g.:
        //   if (2 > 1) then (2 > 1/2, 2 > 1/3, etc.)
        //   so it would only not work for both checks (raw number and conversion) if you divide by n < 1
        if ( (shiftNotMultiply && rawNum > CONVSIZE_MAX / (1ULL << multiplier))
          || (!shiftNotMultiply && rawNum > CONVSIZE_MAX / (multiplier)) )
            return (uint64_t)SizeSig::TooBigResult;
        else if (shiftNotMultiply)
            return rawNum << multiplier;
        else
            return rawNum * multiplier;
    };

    // No unit, or bytes unit
    if (!unitExpected || compareLow(endPtr, "b")) return multiply(0, true);
    // Powers of 2 units
    else if (compareLow(endPtr, "kib")) return multiply(10, true);
    else if (compareLow(endPtr, "mib")) return multiply(20, true);
    else if (compareLow(endPtr, "gib")) return multiply(30, true);
    else if (compareLow(endPtr, "tib")) return multiply(40, true);
    else if (compareLow(endPtr, "pib")) return multiply(50, true);
    else if (compareLow(endPtr, "eib")) return multiply(60, true);
    // Powers of 10 units
    else if (compareLow(endPtr, "kb"))
        return (alwaysBinaryUnits)? multiply(10, true) : multiply(1'000, false);
    else if (compareLow(endPtr, "mb"))
        return (alwaysBinaryUnits)? multiply(20, true) : multiply(1'000'000, false);
    else if (compareLow(endPtr, "gb"))
        return (alwaysBinaryUnits)? multiply(30, true) : multiply(1'000'000'000, false);
    else if (compareLow(endPtr, "tb"))
        return (alwaysBinaryUnits)? multiply(40, true) : multiply(1'000'000'000'000, false);
    else if (compareLow(endPtr, "pb"))
        return (alwaysBinaryUnits)? multiply(50, true) : multiply(1'000'000'000'000'000, false);
    else if (compareLow(endPtr, "eb"))
        return (alwaysBinaryUnits)? multiply(60, true) : multiply(1'000'000'000'000'000'000ULL, false);
    
    return (uint64_t)SizeSig::InvalidUnit;
}

// Instead of a random "true" that could mean either success or failure
// Mostly important in return values to stop ambiguity
enum class ErrorState : uint8_t
{
    Failure, Success, NotFound
};

// Returns read buffer and error state
// It prints an error message on failure too
inline std::pair<std::vector<uint8_t>, ErrorState> readFile(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
    {
        std::cerr << "failed to open file for reading \"" << path << "\"\n";
        return {{}, ErrorState::Failure};
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        std::cerr << "failed to seek file end for reading \"" << path << "\"\n";
        fclose(file);
        return {{}, ErrorState::Failure};
    }

    size_t fileSize = 0;
    // Scope because i don't need the longSize variable
    {
        long longSize = ftell(file);
        if (longSize < 0)
        {
            std::cerr << "failed to get file size for reading \"" << path << "\"\n";
            fclose(file);
            return {{}, ErrorState::Failure};
        }
        else fileSize = (size_t)longSize;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        std::cerr << "failed to reset file seek pointer for reading \"" << path << "\"\n";
        fclose(file);
        return {{}, ErrorState::Failure};
    }

    std::vector<uint8_t> buffer;
    if (fileSize > 0)
    {
        buffer.resize(fileSize);
        size_t readSize = fread(buffer.data(), 1, fileSize, file);
        if (readSize != fileSize)
        {
            std::cerr << "failed to read file \"" << path << "\"\n";
            fclose(file);
            return {{}, ErrorState::Failure};
        }
    }

    if (fclose(file) != 0)
    {
        std::cerr << "failed to close file after reading \"" << path << "\"\n";
        return {{}, ErrorState::Failure};
    }
    return {buffer, ErrorState::Success};
}

// Returns error state
// It prints an error message on failure too
inline ErrorState writeFile(const char* path, const uint8_t* pBuffer, size_t bufferSize)
{
    FILE* file = fopen(path, "wb");
    if (file == nullptr)
    {
        std::cerr << "failed to open file for writing \"" << path << "\"\n";
        return ErrorState::Failure;
    }

    if (bufferSize != 0)
    {
        size_t writtenSize = fwrite(pBuffer, 1, bufferSize, file);
        if (writtenSize != bufferSize)
        {
            std::cerr << "failed to write file \"" << path << "\"\n";
            fclose(file);
            return ErrorState::Failure;
        }
    }

    if (fclose(file) != 0)
    {
        std::cerr << "failed to close file after writing \"" << path << "\"\n";
        return ErrorState::Failure;
    }
    return ErrorState::Success;
}

inline uint32_t crc32(const void* data, size_t length)
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
