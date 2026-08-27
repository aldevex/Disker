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
    None, // Just "nothing" instruction
    Error, // (INTERNAL USAGE ONLY) for signaling an invalid instruction

    SetYes, // allyes/manyes
    SetBinary, // binary/decimal instructions (for gb/mb/etc. units)

    OpenDisk, // openvd/openphd
    SetFormat, // Set disk format (MBR/GDT) [and optional initial partition count]
    SetPart, // Set partition details
    SetBoot, // Set bootloader binary in (and after) MBR, VBR, or at the UEFI default bootloader path

    OpenPart, // Open a partition to set files or sectors
    SetPath, // Set Path (create/write/remove a directory or file)

    ChangeDir, // Change Directory (inside the fs)
    List, // List (list files and directories inside the current fs directory)
    Move, // Move (move a file to a different directory inside the fs)
    Copy, // Copy (copy a file to a different directory inside the fs)

    Whats, // Get details about a virtual/physical disk, partition, or currently opened subjects
    Save, // Save the edits to the opened disk
    Exit // Exit Disker (Stop, Exit, Quit)
};

/*
enum class SubInsType : uint16_t
{
    None,

    Size, // Size in bytes
    SectSize, // Sector size in bytes
    Sects, // Sector count
    Clusts, // Cluster count
    Parts, // Partition count
    SPC, // Sectors per Cluster

    FS, // File System
    Type, // Type
    GUID, // GUID
    Align, // Alignment
    Label // Label
};

// Sub-Instruction
struct SubIns
{
    SubInsType type = SubInsType::None;
    std::string op1, op2;
    
    SubIns() = default;
    SubIns(SubInsType type, std::string_view op1, std::string_view op2)
        : type(type), op1(op1), op2(op2)
    {}
};
*/

union InsInfo
{
    bool switchValue;
    struct {
        uint64_t size,
                sectorCount,
                sectorSize,
                physicalSectorSize;
    } openDisk;

    InsInfo()
        { memset(this, 0, sizeof(InsInfo)); }
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

// Uncomment this after switching NONE_INS to ERROR_INS in internal functions (return to caller excluded)
//const Ins NONE_INS = Ins();
const Ins ERROR_INS = Ins(InsType::Error, InsInfo());

// Returns parsed instruction, and (true) if instruction has more lines (^ or \ at end)
// Takes line vew, and pointer to instruction to continue parsing if multi-line (nullptr otherwise)
std::pair<Ins, bool> parseIns(const std::string_view lineView, const Ins continueIns)
{
    // Utility functions used only inside this function
    static auto cutLine = [&]() -> std::vector<std::string_view>
    {
        std::vector<std::string_view> results;
        for (size_t i = 0; i < lineView.length(); i++)
        {
            if (!isspace(lineView[i]))
            {
                size_t startI = i;
                for (; i < lineView.length(); i++)
                    if (isspace(lineView[i])) break;
                size_t afterEndI = i;
                std::string_view segmentView = lineView.substr(startI, afterEndI - startI);
                if (!segmentView.empty()) results.push_back(segmentView);
            }
        }
        return results;
    };
    static auto compareFull = [](std::string_view anyCaseAnyLenView, std::string_view lowerCaseRequiredView)
                                -> bool
    {
        return (
            (anyCaseAnyLenView.length() == lowerCaseRequiredView.length())
            && !strncmp(Utils::lowerStr(anyCaseAnyLenView).c_str(), lowerCaseRequiredView.data(),
                        lowerCaseRequiredView.length())
        );
    };
    static auto printInvalidInsError = [&](std::string_view reason, std::string_view specifiedText) -> void
    {
        if (specifiedText.empty())
            std::cerr << "invalid instruction (" << reason << ") at \"" << lineView << "\"\n";
        else
            std::cerr << "invalid instruction (" << reason << ") \"" << specifiedText
                                                            << "\" at \"" << lineView << "\"\n";
    };
    // Checks strToSize() result and prints errors accordingly
    static auto checkAndPrintErroneousSize = [](uint64_t val, std::string_view seg1, std::string_view seg2,
                                                std::string_view noNumberErrorText,
                                                std::string_view tooBigResultErrorText) -> void
    {
        using Utils::SizeSig;
        if (val == (uint64_t)SizeSig::NoNumber) printInvalidInsError(noNumberErrorText, seg1);
        else if (val == (uint64_t)SizeSig::InvalidNumber) printInvalidInsError("invalid number", seg2);
        else if (val == (uint64_t)SizeSig::NoUnit) printInvalidInsError("no unit", seg2);
        else if (val == (uint64_t)SizeSig::InvalidUnit) printInvalidInsError("invalid unit", seg2);
        else if (val == (uint64_t)SizeSig::TooBigResult) printInvalidInsError(tooBigResultErrorText, seg2);
    };
    // Parses sector count arguments if they are for sector count & sets pAlreadyGotSectorCountRelated to true
    // Returns value and error state
    static auto ifSectorCountGetIt = [](std::string_view seg1, std::string_view seg2,
                                        bool* pAlreadyGotSectorCountRelated)
                                            -> std::pair<uint64_t, Utils::ErrorState>
    {
        // Parse e.g. sects 524,288
        uint64_t val = (uint64_t)Utils::SizeSig::NoNumber;
        Utils::ErrorState errorState = Utils::ErrorState::NotFound;
        if (compareFull(seg1, "sects"))
        {
            if (*pAlreadyGotSectorCountRelated)
            {
                printInvalidInsError("size or sector count is already given", seg1);
                errorState = Utils::ErrorState::Failure;
            }
            else
            {
                *pAlreadyGotSectorCountRelated = true;
                val = Utils::strToSize(seg2, false, globalState.binaryUnits);
                checkAndPrintErroneousSize(val, seg1, seg2,
                                            "expected sector count argument",
                                            "sector count is too big");
                if (val > (uint64_t)Utils::SizeSig::LEAST_ERROR) errorState = Utils::ErrorState::Failure;
                else errorState = Utils::ErrorState::Success;
            }
        }
        return {val, errorState};
    };

    Ins resultIns = continueIns; // If start of line, continueIns would be INS_NONE so it's ok to use it anyway
    bool moreLines = false;
    std::vector<std::string_view> segments = cutLine();
    
    ///////////////////////////////////////////
    // Skips
    ///////////////////////////////////////////
    // All whitespace no segments
    if (segments.empty())
        resultIns = Ins(InsType::None, InsInfo());
    // Comment
    else if (segments[0].size() >= 2 && segments[0][0] == '/' && segments[0][1] == '/')
        resultIns = Ins(InsType::None, InsInfo());

    ///////////////////////////////////////////
    // Continue multi-line instruction
    ///////////////////////////////////////////
    else if (pContinueIns != nullptr)
    {
        return {resultIns, moreLines};
    }

    ///////////////////////////////////////////
    // Start of new instruction
    ///////////////////////////////////////////
    // Help
    else if (compareFull(segments[0], "help") && segments.size() > 1)
    {
        std::cout <<
            "Help string idfk what to put i will add later\n"
            ""
            ""
            ""
        << std::endl;
        resultIns = Ins(InsType::Exit, InsInfo());
    }
    // Stop, Exit, QUit
    else if ((compareFull(segments[0], "stop")
            || compareFull(segments[0], "exit")
            || compareFull(segments[0], "quit")
            ) && segments.size() > 1)
    {
        resultIns = Ins(InsType::Exit, InsInfo());
    }
    // allyes
    else if (compareFull(segments[0], "allyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            return {ERROR_INS, false};
        }
        else resultIns = Ins(InsType::AllYes, InsInfo());
    }
    // manyes
    else if (compareFull(segments[0], "manyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidError("expected no operands", "");
            return {NONE_INS, false};
        }
        else resultIns = Ins(InsType::ManYes, InsInfo());
    }
    // openvd
    else if (compareFull(segments[0], "openvd"))
    {
        using Utils::SizeSig;
        // Maximum line would be
        //   openvd x.img size 32gib (OR sects N) sectsize 4096B
        // Which is 6 segments (5 operands)
        if (segments.size() > 6)
        {
            printInvalidError("expected 5 (or less) operands only", "");
            return {NONE_INS, false};
        }
        // Parse e.g. openvd x.img
        bool gotSizeRelated = false; // Got size or sector count
        bool gotSectSize = false;
        for (size_t i = 0; i < segments.size(); i++)
        {
            // Parse e.g. size 32gib
            if (compareFull(segments[i], "size"))
            {
                if (gotSizeRelated)
                {
                    printInvalidError("size or sector count is already given", segments[i]);
                    return {NONE_INS, false};
                }
                else gotSizeRelated = true;
                if (i +1 == segments.size())
                {
                    printInvalidError("expected size argument", segments[i]);
                    return {NONE_INS, false};
                }
                uint64_t val = Utils::strToSize(segments[i +1], true, globalState.binaryUnits);
                bool error = checkPrintErroneousNum(val, segments[i +1], "too many bytes");
                if (error) return {NONE_INS, false};
                else resultIns.subIns.push_back(SubIns(SubInsType::Size, segments[i +1], ""));
            }
            // Parse e.g. sects 524,288
            // Hi here new code for parsing bs idk if it's really good, it looks ugly
            auto sectorCountPair = ifSectorCountGetIt(segments[i],
                                                        (i +1 != segments.size())?  segments[i +1] : "",
                                                        &gotSizeRelated);
            if (sectorCountPair.second == Utils::ErrorState::Failure)
                return {ERROR_INS, false};
            else if (sectorCountPair.second == Utils::ErrorState::Success)
                resultIns.info.openDisk.sectorCount = sectorCountPair.first;
            /*
            else if (compareFull(segments[i], "sects"))
            {
                if (gotSizeRelated)
                {
                    printInvalidError("size or sector count is already given", segments[i]);
                    return {NONE_INS, false};
                }
                else gotSizeRelated = true;
                if (i +1 == segments.size())
                {
                    printInvalidError("expected sector count argument", segments[i]);
                    return {NONE_INS, false};
                }
                uint64_t val = Utils::strToSize(segments[i +1], false, globalState.binaryUnits);
                bool error = checkPrintErroneousNum(val, segments[i +1], "sector count is too big");
                if (error) return {NONE_INS, false};
                else resultIns.subIns.push_back(SubIns(SubInsType::Sects, segments[i +1], ""));
            }
            */
            // Parse e.g. sectsize 4096B
            else if (compareFull(segments[i], "sectsize"))
            {
                if (gotSectSize)
                {
                    printInvalidError("sector size is already given", segments[i]);
                    return {NONE_INS, false};
                }
                else gotSectSize = true;
                if (i +1 == segments.size())
                {
                    printInvalidError("expected sector size argument", segments[i]);
                    return {NONE_INS, false};
                }
                uint64_t val = Utils::strToSize(segments[i +1], true, globalState.binaryUnits);
                bool error = checkPrintErroneousNum(val, segments[i +1], "sector size is too big");
                if (error) return {NONE_INS, false};
                else resultIns.subIns.push_back(SubIns(SubInsType::SectSize, segments[i +1], ""));
            }
        }
        // Ahuuuuuu i have to make it check if user gave 0 as a size/count and which of either they gave
        // Make lambda functions for each instruction and sub-instruction because this pmo
        if (gotSizeRelated)
        {

        }
        if (gotSectSize)
        {

        }
        resultIns.info.openDisk.size = 34893246324;
        resultIns = Ins(InsType::OpenVD, segments[2], "", nullptr);
    }
    
    ///////////////////////////////////////////
    // Unsupported instruction
    ///////////////////////////////////////////
    if (resultIns.type == InsType::None)
    {
        std::cerr << "invalid instruction (unsupported) \"" << lineView << "\"\n";
    }
    
    ///////////////////////////////////////////
    // Return
    ///////////////////////////////////////////
    return {resultIns, moreLines};
}

// Validates commands file and returns instructions vector (empty if contains 1+ errors)
std::vector<Ins> parseFile(const char* path)
{
    std::vector<Ins> results;
    std::stringstream cmdFileStream;
    // Scope for temporary variable
    {
        std::pair<std::string, ErrorState> result = Utils::readFile(path);
        if (result.second != ErrorState::Success) return results;
        else cmdFileStream = std::stringstream(result.first);
    }
    std::string line;

    Ins ins = NONE_INS;
    bool moreLines = false;
    size_t errorCount = 0;
    while (std::getline(cmdFileStream, line))
    {
        // 10 errors max so user isn't overwhelmed
        if (errorCount == 10) break;
        // Remove carriage return from WINDOOOWS FILES because getline doesn't
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Parsed result pair
        std::pair<Ins, bool> parsed = {ins, moreLines};
        // First line of instruction
        if (!moreLines) parsed = parseIns(line, NONE_INS);
        // More lines to same instruction
        else parsed = parseIns(line, ins);
        // Open parsed result pair
        ins = parsed.first;
        moreLines = parsed.second;

        // Error happened
        if (ins.type == InsType::Error)
        {
            errorCount++;
            results.clear();
            // Don't return but keep checking more errors
        }

        // No more lines (& no error happened) (& not a None (used for skipping whitespace/comments))
        if (!moreLines && errorCount == 0 && ins.type != InsType::None) results.push_back(ins);
    }
    return results;
}

Ins parseTerminal()
{
    std::string line;
    std::getline(std::cin, line);

    Ins ins = NONE_INS;
    bool moreLines = false;
    do {
        // Parsed result pair
        std::pair<Ins, bool> parsed = {NONE_INS, false};
        // First line of instruction
        if (!moreLines) parsed = parseIns(line, NONE_INS);
        // More lines to same instruction
        else parsed = parseIns(line, ins);
        // Open parsed result pair
        ins = parsed.first;
        moreLines = parsed.second;

        // Error happened
        if (ins.type == InsType::Error) return NONE_INS;

        // No more lines
        if (!moreLines) return ins;
    } while (moreLines);

    return Ins();
}

}
