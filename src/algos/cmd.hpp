#pragma once
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>
#include <sstream>
#include <string>
#include <string_view>
#include <iostream>
#include "../algos/utils.hpp"
namespace Cmd {

// Instruction type
enum class InsType : uint16_t
{
    None,

    SetYes, // allyes/manyes
    SetBinary, // binary/decimal instructions (for gb/mb/etc. units)

    OpenDisk, // openvd/openphd
    SetFormat, // Set disk format (MBR/GDT) [and optional initial partition count]
    SetPart, // Set partition details
    SetBoot, // Set bootloader binary in (and after) MBR, VBR, or at the UEFI default bootloader path
    OpenPart, // Open a partition to set files or sectors

    SetFSI, // Set File System Item (file or directory)
    DelFSI, // Delete File System Item (file or directory)
    CopyFSI, // Copy File System Item (file or directory)
    MoveFSI, // Move File System Item (file or directory)
    RenameFSI, // Rename File System Item (file or directory)

    ListFSIs, // List File System Items (files and directories)
    ChangeDir, // Change the current directory
    
    Whats, // Get details about a virtual/physical disk, partition, or currently opened subjects
    Save, // Save the edits to the opened disk
    Exit // Exit Disker (Stop, Exit, Quit)
};

struct InsInfo
{
    std::string textBuffer1, textBuffer2;
    union {
        bool switchValue;
        struct {
            uint64_t size,
                    sectorSize,
                    physicalSectorSize;
            bool isReal;
            void setPath(InsInfo* pInsInfo, std::string_view pathView)
                { pInsInfo->textBuffer1 = pathView; }
            std::string_view getPath(const InsInfo* pInsInfo) const
                { return pInsInfo->textBuffer1; }
        } openDisk;
    };

    InsInfo()
    {
        memset(this +offsetof(InsInfo, textBuffer2), 0,
                sizeof(InsInfo) -sizeof(textBuffer2));
    }
};

// Instruction
struct Ins
{
    InsType type = InsType::None;
    InsInfo info;
    
    Ins() = default;
    Ins(InsType type, InsInfo info)
        : type(type), info(info)
    {}
};

const Ins NONE_INS = Ins();

// Returns parsed instruction (None on error), and error state
// Prints error in instruction on encounter
std::pair<Ins, Utils::ErrorState> parseIns(const std::string_view insView, bool alwaysBinaryUnits)
{
    // Utility functions used only inside this function
    // Cuts line into segments without spaces
    static auto cutLine = [&]() -> std::vector<std::string_view>
    {
        std::vector<std::string_view> results;
        for (size_t i = 0; i < insView.length(); i++)
        {
            if (!isspace((unsigned char)insView[i]))
            {
                size_t startI = i;
                for (; i < insView.length(); i++)
                    if (isspace((unsigned char)insView[i])) break;
                size_t afterEndI = i;
                std::string_view segmentView = insView.substr(startI, afterEndI - startI);
                if (!segmentView.empty()) results.push_back(segmentView);
            }
        }
        return results;
    };
    // Converts the first string (internally) to lower case then compares it to the other string
    // Returns true if equal
    static auto compareLow = [](std::string_view anyCaseAnyLenView, std::string_view lowerCaseRequiredView)
                                -> bool
    {
        return (
            (anyCaseAnyLenView.length() == lowerCaseRequiredView.length())
            && !strncmp(Utils::lowerStr(anyCaseAnyLenView).c_str(), lowerCaseRequiredView.data(),
                        lowerCaseRequiredView.length())
        );
    };
    // Prints: invalid instruction "$insView" ($reason "$specifiedText")
    static auto printInvalidInsError = [&](std::string_view reason, std::string_view specifiedText) -> void
    {
        if (specifiedText.empty())
            std::cerr << "invalid instruction \"" << insView << "\" (" << reason << ")\n";
        else
            std::cerr << "invalid instruction \"" << insView << "\" (" << reason
                        << "\"" << specifiedText << "\")\n";
    };
    // Checks if size (strToSize() result) is part of SizeSig error signals and prints errors accordingly
    // Returns true if the size is erroneous
    static auto checkAndPrintErroneousSize = [](uint64_t val, std::string_view seg1, std::string_view seg2,
                                                std::string_view tooBigResultErrorText) -> bool
    {
        // This shouldn't be possible but I'm gonna put it anyway
        if (val == (uint64_t)Utils::SizeSig::NoNumber)
            printInvalidInsError("no number", seg1);

        else if (val == (uint64_t)Utils::SizeSig::InvalidNumber)
            printInvalidInsError("invalid number", seg2);

        else if (val == (uint64_t)Utils::SizeSig::NoUnit)
            printInvalidInsError("no unit", seg2);

        else if (val == (uint64_t)Utils::SizeSig::UnacceptableZero)
            printInvalidInsError("zero is unacceptable", seg2);

        else if (val == (uint64_t)Utils::SizeSig::TooBigResult)
            printInvalidInsError(tooBigResultErrorText, seg2);

        else if (val == (uint64_t)Utils::SizeSig::InvalidUnit)
            printInvalidInsError("invalid unit", seg2);

        else return false;
        return true;
    };
    // Creates generic switch instruction info (i made this for one-liners)
    static auto makeSwitchInfo = [](bool value) -> InsInfo
    {
        InsInfo result;
        result.switchValue = value;
        return result;
    };
    
    std::vector<std::string_view> segments = cutLine();
    Ins resultIns = NONE_INS; // Keep type as None on errors
    bool error = false; // Set this on error discovery
    
    // All whitespace no segments
    if (segments.empty())
        resultIns = Ins(InsType::None, InsInfo());
    // Help
    else if (compareLow(segments[0], "help"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else
        {
            std::cout <<
                "Help string idfk what to put i will add later\n"
                ""
                ""
                ""
            << std::endl;
            resultIns = Ins(InsType::Exit, InsInfo());
        }
    }
    
    // allyes
    else if (compareLow(segments[0], "allyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetYes, makeSwitchInfo(true));
    }
    // manyes
    else if (compareLow(segments[0], "manyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetYes, makeSwitchInfo(false));
    }
    
    // binary
    else if (compareLow(segments[0], "binary"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetBinary, makeSwitchInfo(true));
    }
    // decimal
    else if (compareLow(segments[0], "decimal"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetBinary, makeSwitchInfo(false));
    }

    // openvd
    else if (compareLow(segments[0], "openvd"))
    {
        InsInfo info;
        // e.g. openvd x.img size 32gib sectsize 4096B
        // Parse e.g. openvd x.img
        if (segments.size() == 1)
        {
            printInvalidInsError("expected a file name", "");
            error = true;
        }
        else
        {
            info.openDisk.setPath(&info, segments[1]);
            info.openDisk.isReal = false;
        } // The following loop checks "!error" so consider it part of this "else" statement
        for (size_t i = 0; !error && i < segments.size(); i+= 2)
        {
            // Parse e.g. size 32gib
            if (compareLow(segments[i], "size"))
            {
                if (info.openDisk.size != 0)
                {
                    printInvalidInsError("repeated size", "");
                    error = true;
                }
                else if (i +1 == segments.size())
                {
                    printInvalidInsError("expected size argument", segments[i]);
                    error = true;
                }
                else
                {
                    uint64_t val = Utils::strToSize(segments[i +1], true, alwaysBinaryUnits, true);
                    error = checkAndPrintErroneousSize(val, segments[i], segments[i +1],
                                                            "size is too big");
                    if (!error) info.openDisk.size = val;
                }
            }
            // Parse e.g. sectsize 4096B
            else if (compareLow(segments[i], "sectsize"))
            {
                if (info.openDisk.sectorSize != 0)
                {
                    printInvalidInsError("repeated sector size", "");
                    error = true;
                }
                else if (i +1 == segments.size())
                {
                    printInvalidInsError("expected sector size argument", segments[i]);
                    error = true;
                }
                else
                {
                    uint64_t val = Utils::strToSize(segments[i +1], true, alwaysBinaryUnits, true);
                    error = checkAndPrintErroneousSize(val, segments[i], segments[i +1],
                                                            "sector size is too big");
                    if (!error)
                    {
                        info.openDisk.sectorSize = val;
                        info.openDisk.physicalSectorSize = val;
                    }
                }
            }
            // Other invalid subinstruction
            else
            {
                printInvalidInsError("invalid sub-instruction", segments[i]);
                error = true;
            }
        }
        if (info.openDisk.size == 0)
        {
            info.openDisk.size = Utils::strToSize("64MiB", true, true, true);
        }
        if (info.openDisk.sectorSize == 0)
        {
            info.openDisk.sectorSize = 512;
            info.openDisk.physicalSectorSize = 512;
        }
        resultIns = Ins(InsType::OpenDisk, info);
    }
    
    // Stop, Exit, QUit
    else if (compareLow(segments[0], "stop")
            || compareLow(segments[0], "exit")
            || compareLow(segments[0], "quit"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::Exit, InsInfo());
    }
    
    // Unsupported instruction
    if (!error && resultIns.type == InsType::None) printInvalidInsError("unsupported instruction", "");

    // Return
    return {resultIns, (!error)? Utils::ErrorState::Success : Utils::ErrorState::Failure};
}

// Validates commands file and returns instructions vector (empty if contains errors)
std::vector<Ins> parseFile(const char* path, bool alwaysBinaryUnits)
{
    // Returns:
    //   1. Clean string view (no comments or multi-line continue symbol)
    //   2. Bool (true = instruction continues into the next line (mult-line))
    static auto cleanLine = [](std::string_view lineView) -> std::pair<std::string_view, bool>
    {
        // Remove carriage return from WINDOOOWS FILES because getline doesn't
        if (!lineView.empty() && lineView.back() == '\r') lineView.remove_suffix(1);
        // Remove comment
        for (size_t i = 0; i < lineView.size(); i++)
        {
            if (lineView[i] == '#' // Comment
            && (i == 0 || isspace((unsigned char)lineView[i -1])) ) // Nothing or space before it
                                                    //   (hashtag may be part of instructions)
            {
                lineView.remove_suffix(i +1);
                break;
            }
        }
        // Check for multi-line continue (and remove it)
        bool continueNextLine = false;
        for (size_t i = lineView.size() -1; i != SIZE_MAX; i--)
        {
            if (isspace((unsigned char)lineView[i])) continue;
            else if (lineView[i] != '\\') break; // Normal character (not space nor the symbol)
            else // Multi-line continue symbol
            {
                continueNextLine = true;
                lineView.remove_suffix(lineView.size() -i);
                break;
            }
        }
        return {lineView, continueNextLine};
    };

    std::vector<Ins> results;
    size_t errorCount = 0;

    // Read the file
    std::stringstream fileStream;
    // Scope for temporary variable
    {
        std::pair<std::vector<uint8_t>, Utils::ErrorState> fileResult = Utils::readFile(path);
        if (fileResult.second != Utils::ErrorState::Success) return results;
        else fileStream = std::stringstream(std::string(
            (char*)fileResult.first.data(), fileResult.first.size()
        ));
    }

    // Loop over the file
    std::string insString = "";
    std::string line = "";
    bool expectingNextLine = false;
    while (std::getline(fileStream, line) || expectingNextLine) // Line = "" if nothing left in stream
    {
        // If expectingNextLine && line == "": expectingNextLine = false
        //   then the loop stops even if the last line has a redundant multi-line symbol
        std::pair<std::string_view, bool> cleanResult = cleanLine(line);
        line = cleanResult.first;
        expectingNextLine = cleanResult.second;
        // Append in all cases + space on both sides padding
        insString += ' ';
        insString.append(cleanResult.first);
        insString += ' ';
        // Continue adding if expecting a new line
        if (expectingNextLine) continue;
        
        // Parse
        Ins ins = NONE_INS;
        Utils::ErrorState errorState = Utils::ErrorState::Failure;
        std::pair<Ins, Utils::ErrorState> parseResult = parseIns(insString, alwaysBinaryUnits);
        insString = ""; // Clear for next instruction
        ins = parseResult.first;
        errorState = parseResult.second;
        // Error happened
        if (errorState != Utils::ErrorState::Success)
        {
            errorCount++;
            results.clear();
            // Don't return, but keep checking more errors
        }

        // Add to results if: no error happened & not a None (used for skipping whitespace/comments)
        if (errorCount == 0 && ins.type != InsType::None)
        {
            results.push_back(ins);
            if (ins.type == InsType::SetBinary) alwaysBinaryUnits = ins.info.switchValue;
        }
        // 10 errors max so user isn't overwhelmed
        else if (errorCount == 10) break;
    }

    return results;
}

std::vector<Ins> parseDirectTerminal(size_t argC, const char** argV, bool alwaysBinaryUnits)
{
    // For passing instructions via direct arguments
    //   e.g. "disker - openvd x.img - format mbr parts 1 - part 1 fs FAT32 size 16gb"
    
    std::vector<Ins> results;
    size_t errorCount = 0;
    
    // Loop over argument segments
    std::string insString = "";
    for (size_t i = 0; i < argC; i++)
    {
        // Instruction separator for direct argument passing
        if (!strcmp(argV[i], "-"))
        {
            // Add argument segments with padding together
            // This is kinda dumb because parseIns cuts it back again but idc
            insString = ""; // Clear previous string
            for (size_t j = 0; j < i; j++)
            {
                insString.append(argV[j]);
                insString += ' ';
            }
            // Parse
            Ins ins = NONE_INS;
            Utils::ErrorState errorState = Utils::ErrorState::Failure;
            std::pair<Ins, Utils::ErrorState> parseResult = parseIns(insString, alwaysBinaryUnits);
            ins = parseResult.first;
            errorState = parseResult.second;
            // Error happened
            if (errorState != Utils::ErrorState::Success)
            {
                errorCount++;
                results.clear();
                // Don't return, but keep checking more errors
            }
            // Add to results if: no error happened & not a None (used for skipping whitespace/comments)
            if (errorCount == 0 && ins.type != InsType::None)
            {
                results.push_back(ins);
                if (ins.type == InsType::SetBinary) alwaysBinaryUnits = ins.info.switchValue;
            }
        }
        // 10 errors max so user isn't overwhelmed
        if (errorCount == 10) break;
    }

    return results;
}

Ins parseTerminal(bool alwaysBinaryUnits)
{
    std::string line;
    std::getline(std::cin, line);
    return parseIns(line, alwaysBinaryUnits).first;
}

}
