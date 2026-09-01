#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include "../algos/utils.hpp"
//#include "structs/generic.hpp"
//#include "structs/mbr.hpp"
//#include "structs/gpt.hpp"
//#include "structs/fat32.hpp"
#include "../algos/inss.hpp"
#include "state.hpp"

std::vector<Inss::Ins> parseFile(const char* path, bool alwaysBinaryUnits);
std::vector<Inss::Ins> parseDirectArgs(size_t argC, const char* const* argV, bool alwaysBinaryUnits);
Inss::Ins parseTerminalLine(bool alwaysBinaryUnits);

int TUIMain(int argC, char** argV)
{
    // Instructions via terminal
    if (argC == 1)
    {
        std::cout << "type \"help\" for help | type \"quit\" to exit the program" << std::endl;
        while (true)
            applyIns(parseTerminalLine(state.alwaysBinaryUnits));
    }
    else
    {
        // Help
        if (argC == 2 && Utils::compareLow(argV[1], "help"))
        {
            Utils::printHelp();
        }
        // Instructions via file
        else if (argC == 3 && Utils::compareLow(argV[1], "do"))
        {
            std::vector<Inss::Ins> instructions = parseFile(argV[2], state.alwaysBinaryUnits);
            for (const Inss::Ins& ins : instructions)
                applyIns(ins);
        }
        // Instructions via direct arguments
        else if (!strncmp(argV[1], "-", 1))
        {
            std::vector<Inss::Ins> instructions = parseDirectArgs(argC, argV, state.alwaysBinaryUnits);
            for (const Inss::Ins& ins : instructions)
                applyIns(ins);
        }
        // Invalid args
        else
        {
            std::cerr << "invalid arguments. expected \"do <FILE NAME>\""
                        " or \"-<instruction1> [operands] -[instruction2] [operands]...\"\n"
                        "use \"help\" for help";
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}



// Returns parsed instruction (None on error), and error state
// Prints error in instruction on encounter
std::pair<Inss::Ins, Utils::ErrorState> parseIns(const std::string_view insView, bool alwaysBinaryUnits)
{
    using namespace Inss;
    // Utility functions used only inside this function
    // Cuts line into segments without spaces
    auto cutLine = [&]() -> std::vector<std::string_view>
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
    // Prints: invalid instruction "$insView" ($reason "$specifiedText")
    auto printInvalidInsError = [&](std::string_view reason, std::string_view specifiedText) -> void
    {
        if (specifiedText.empty())
            std::cerr << "invalid instruction \"" << insView << "\" (" << reason << ")\n";
        else
            std::cerr << "invalid instruction \"" << insView << "\" (" << reason
                        << " \"" << specifiedText << "\")\n";
    };
    // Checks if size (strToSize() result) is part of SizeSig error signals and prints errors accordingly
    // Returns true if the size is erroneous
    auto checkAndPrintErroneousSize = [&](uint64_t val, std::string_view seg1, std::string_view seg2,
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
    auto makeSwitchInfo = [](bool value) -> InsInfo
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
        resultIns.type = InsType::InternalSkip;
    // help
    else if (Utils::compareLow(segments[0], "help"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else
        {
            Utils::printHelp();
            resultIns.type = InsType::InternalSkip;
        }
    }
    
    // allyes
    else if (Utils::compareLow(segments[0], "allyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetYes, makeSwitchInfo(true));
    }
    // manyes
    else if (Utils::compareLow(segments[0], "manyes"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetYes, makeSwitchInfo(false));
    }
    
    // binary
    else if (Utils::compareLow(segments[0], "binary"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetBinary, makeSwitchInfo(true));
    }
    // decimal
    else if (Utils::compareLow(segments[0], "decimal"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::SetBinary, makeSwitchInfo(false));
    }

    // openvd
    else if (Utils::compareLow(segments[0], "openvd"))
    {
        // e.g. openvd x.img size 32gib sectsize 4096B
        // Parse e.g. openvd x.img
        if (segments.size() == 1)
        {
            printInvalidInsError("expected a file name", "");
            error = true;
        }
        else
        {
            resultIns.info.openDisk.setPath(&resultIns, segments[1]);
            resultIns.info.openDisk.isReal = false;
        }
        for (size_t i = 2; !error && i < segments.size(); i+= 2)
        {
            // Parse e.g. size 32gib
            if (Utils::compareLow(segments[i], "size"))
            {
                if (resultIns.info.openDisk.size != 0)
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
                    if (!error) resultIns.info.openDisk.size = val;
                }
            }
            // Parse e.g. sectsize 4096B
            else if (Utils::compareLow(segments[i], "sectsize"))
            {
                if (resultIns.info.openDisk.sectorSize != 0)
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
                        resultIns.info.openDisk.sectorSize = val;
                        resultIns.info.openDisk.physicalSectorSize = val;
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
        if (!error)
        {
            if (resultIns.info.openDisk.size == 0)
            {
                resultIns.info.openDisk.size = Utils::strToSize("64MiB", true, true, true);
            }
            if (resultIns.info.openDisk.sectorSize == 0)
            {
                resultIns.info.openDisk.sectorSize = 512;
                resultIns.info.openDisk.physicalSectorSize = 512;
            }
            resultIns.type = InsType::OpenDisk;
        }
    }
    
    // Save
    else if (Utils::compareLow(segments[0], "save"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("expected no operands", "");
            error = true;
        }
        else resultIns = Ins(InsType::Save, InsInfo());
    }
    // Stop, Exit, QUit
    else if (Utils::compareLow(segments[0], "stop")
            || Utils::compareLow(segments[0], "exit")
            || Utils::compareLow(segments[0], "quit"))
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
    else if (resultIns.type == InsType::InternalSkip) resultIns.type = InsType::None;

    // Return
    return {resultIns, (!error)? Utils::ErrorState::Success : Utils::ErrorState::Failure};
}

// Validates commands file and returns instructions vector (empty if contains errors)
std::vector<Inss::Ins> parseFile(const char* path, bool alwaysBinaryUnits)
{
    using namespace Inss;
    // Returns:
    //   1. Clean string view (no comments or multi-line continue symbol)
    //   2. Bool (true = instruction continues into the next line (mult-line))
    auto cleanLine = [](std::string_view lineView) -> std::pair<std::string_view, bool>
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

std::vector<Inss::Ins> parseDirectArgs(size_t argC, const char* const* argV, bool alwaysBinaryUnits)
{
    using namespace Inss;
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

Inss::Ins parseTerminalLine(bool alwaysBinaryUnits)
{
    using namespace Inss;
    std::string line;
    std::getline(std::cin, line);
    return parseIns(line, alwaysBinaryUnits).first;
}
