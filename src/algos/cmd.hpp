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
#include "../utils.hpp"
namespace Cmd {

// Instruction type
enum class InsType : uint16_t
{
    None,
    Skip, // For comments/whitespace

    AllYes, // Accept everything automatically
    ManYes, // Accept manually

    Binary, // Consider gb/mb/etc. as gib/mib/etc.
    Decimal, // Consider gb/mb/etc as actual gigabytes/megabytes/etc.

    OpenVD, // Open Virtual Disk (disk image file)
    OpenPhD, // Open Physical Disk

    Format, // Set disk format (MBR/GDT) [and optional initial partition count]
    Part, // Set partition details

    Boot, // Set bootloader binary in (and after) MBR, VBR, or at the UEFI default bootloader path

    OpenPart, // Open a partition to set files or sectors
    SetP, // Set Path (create a directory, or write a file)
    remP, // Remove path (remove a directory or file)

    CD, // Change Directory (inside the fs)
    Ls, // List (list files and directories inside the current fs directory)
    Mv, // Move (move a file to a different directory inside the fs)
    Cp, // Copy (copy a file to a different directory inside the fs)

    Whats, // Get details about a virtual/physical disk, partition, or currently opened subjects
    Save, // Save the edits to the opened disk
    Exit // Exit Disker (Stop, Exit, Quit)
};

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

// Instruction
struct Ins
{
    InsType type = InsType::None;
    std::string op1, op2;
    std::vector<SubIns> subIns;

    Ins() = default;
    Ins(InsType type, std::string_view op1, std::string_view op2, std::vector<SubIns>* pSubInsVec)
        : type(type), op1(op1), op2(op2)
    {
        if (pSubInsVec != nullptr) subIns = *pSubInsVec;
    }
};

// Used to signal an error too (only internally in this header)
#define NONE_INS Ins(InsType::None, "", "", nullptr)

// Returns parsed instruction, and (true) if instruction has more lines (^ or \ at end)
// Takes line vew, and pointer to instruction to continue parsing if multi-line (nullptr otherwise)
std::pair<Ins, bool> parseIns(const std::string_view lineView, const Ins* pContinueIns)
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
    static auto compareStart = [](std::string_view anyCaseAnyLenView, std::string_view lowerCaseRequiredView)
                                -> bool
    {
        return !strncmp(Utils::lowerStr(anyCaseAnyLenView).c_str(), lowerCaseRequiredView.data(),
                        lowerCaseRequiredView.length());
    };
    static auto compareFull = [](std::string_view anyCaseAnyLenView, std::string_view lowerCaseRequiredView)
                                -> bool
    {
        if (anyCaseAnyLenView.length() != lowerCaseRequiredView.length()) return false;
        else return compareStart(anyCaseAnyLenView, lowerCaseRequiredView);
    };
    static auto printInvalidError = [&](std::string_view reason, std::string_view specifiedText) -> void
    {
        if (specifiedText.empty())
            std::cerr << "invalid instruction (" << reason << ") at \"" << lineView << "\"\n";
        else
            std::cerr << "invalid instruction (" << reason << ") \"" << specifiedText
                                                            << "\" at \"" << lineView << "\"\n";
    };
    // Multi-use functions also used only here
    static auto validateSectCount = []()
    {
        
    };

    // Start of code
    Ins resultIns = (pContinueIns == nullptr)? Ins() : *pContinueIns;
    bool moreLines = false;

    std::vector<std::string_view> segments = cutLine();
    
    ///////////////////////////////////////////
    // Skips
    ///////////////////////////////////////////
    // All whitespace no segments
    if (segments.empty())
        resultIns = Ins(InsType::Skip, "", "", nullptr);
    // Comment
    else if (compareStart(segments[0], "//"))
        resultIns = Ins(InsType::Skip, "", "", nullptr);

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
        resultIns = Ins(InsType::Exit, "", "", nullptr);
    }
    // Stop, Exit, QUit
    else if ((compareFull(segments[0], "stop")
            || compareFull(segments[0], "exit")
            || compareFull(segments[0], "quit")
            ) && segments.size() > 1)
    {
        resultIns = Ins(InsType::Exit, "", "", nullptr);
    }
    // allyes
    else if (compareFull(segments[0], "allyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidError("expected no operands", "");
            return {NONE_INS, false};
        }
        else resultIns = Ins(InsType::AllYes, "", "", nullptr);
    }
    // manyes
    else if (compareFull(segments[0], "manyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidError("expected no operands", "");
            return {NONE_INS, false};
        }
        else resultIns = Ins(InsType::ManYes, "", "", nullptr);
    }
    // openvd
    else if (compareFull(segments[0], "openvd"))
    {
        using Utils::SizeSig;
        // Maximum line would be
        //   openvd x.img size 32gib (OR sects N) sectsize 4096
        // Which is 6 segments (5 operands)
        if (segments.size() > 6)
        {
            printInvalidError("expected 5 (or less) operands only", "");
            return {NONE_INS, false};
        }
        // Parse e.g. openvd x.img
        resultIns = Ins(InsType::OpenVD, segments[2], "", nullptr);
        bool gotSizeRelated = false; // Got size or sector count
        bool gotSectSize = false;
        for (size_t i = 0; i < segments.size(); i++)
        {
            // Parse e.g. size 32gib
            if (compareFull(segments[i], "size"))
            {
                if (i +1 == segments.size())
                {
                    printInvalidError("expected size argument", segments[i]);
                    return {NONE_INS, false};
                }
                bool error = true;
                size_t val = Utils::strToSize(segments[i +1], true, globalState.binaryUnits);
                if ((SizeSig)val == SizeSig::InvalidNumber) printInvalidError("invalid number", segments[i +1]);
                else if ((SizeSig)val == SizeSig::NoUnit) printInvalidError("no unit", segments[i +1]);
                else if ((SizeSig)val == SizeSig::InvalidUnit) printInvalidError("invalid unit", segments[i +1]);
                else if ((SizeSig)val == SizeSig::TooBigResult) printInvalidError("too many bytes", segments[i +1]);
                else error = false;
                if (error) return {NONE_INS, false};
                else resultIns.subIns.push_back(SubIns(SubInsType::Size, segments[i +1], ""));
            }
            // Parse e.g. sects 524,288
            else if (compareFull(segments[i], "sects"))
            {
                ...
                validateSectCount();
            }
            // Parse e.g. sectsize 4096
            else if (compareFull(segments[i], "sectsize"))
            {
                ...
            }
        }
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
    std::stringstream cmdFileStream = std::stringstream(File::read(path));
    std::string line;

    Ins ins = Ins();
    bool moreLines = false;
    size_t errorCount = 0;
    while (std::getline(cmdFileStream, line))
    {
        // 10 errors max so user isn't overwhelmed
        if (errorCount > 10) break;
        // Remove carriage return from WINDOOOWS FILES because getline doesn't
        if (!line.empty() && line.back() == '\r') line.pop_back();

        // Parsed result pair
        std::pair<Ins, bool> parsed = {ins, moreLines};
        // First line of instruction
        if (!moreLines) parsed = parseIns(line, nullptr);
        // More lines to same instruction
        else parsed = parseIns(line, &ins);
        // Open parsed result pair
        ins = parsed.first;
        moreLines = parsed.second;

        // Error happened
        if (ins.type == InsType::None)
        {
            errorCount++;
            results.clear();
            // Don't return but keep checking more errors
        }

        // No more lines (& no error happened) (& not a Skip)
        if (!moreLines && errorCount == 0 && ins.type != InsType::Skip) results.push_back(ins);
    }
    return results;
}

Ins parseTerminal()
{
    std::string line;
    std::getline(std::cin, line);

    Ins ins = Ins();
    bool moreLines = false;
    do {
        // Parsed result pair
        std::pair<Ins, bool> parsed = {ins, moreLines};
        // First line of instruction
        if (!moreLines) parsed = parseIns(line, nullptr);
        // More lines to same instruction
        else parsed = parseIns(line, &ins);
        // Open parsed result pair
        ins = parsed.first;
        moreLines = parsed.second;

        // Error happened or Skip
        if (ins.type == InsType::None || ins.type == InsType::Skip) return NONE_INS;

        // No more lines
        if (!moreLines) return ins;
    } while (moreLines);

    return Ins();
}

}
