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
        if (argC == 2)
        {
            // Help
            if (Utils::compareLow(argV[1], "help"))
            {
                Utils::printHelp();
            }
            // Instructions via file
            else
            {
                std::vector<Inss::Ins> instructions = parseFile(argV[2], state.alwaysBinaryUnits);
                for (const Inss::Ins& ins : instructions)
                    applyIns(ins);
            }
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
            std::cerr << "invalid arguments. expected \"<FILE NAME>\""
                        " or \"-<instruction1> <arguments> -[instruction2] [arguments]...\"\n"
                        "use \"help\" for help";
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}



// Returns parsed instruction (None on error), and error state
// Prints error in instruction on encounter
std::pair<Inss::Ins, Utils::ErrorState> parseIns(const std::string_view insView, bool alwaysBinaryUnits,
                                                bool callerIsParsingASingleLine)
{
    using namespace Inss;
    // Prints: invalid instruction "$insView" ($reason "$specifiedText")
    auto printInvalidInsError = [&](std::string_view reason, std::string_view specifiedText) -> void
    {
        if (!callerIsParsingASingleLine)
        {
            if (specifiedText.empty())
                std::cerr << "invalid instruction \"" << insView << "\" (" << reason << ")\n";
            else
                std::cerr << "invalid instruction \"" << insView << "\" (" << reason
                            << " \"" << specifiedText << "\")\n";
        }
        else
        {
            if (specifiedText.empty())
                std::cerr << "invalid instruction (" << reason << ")\n";
            else
                std::cerr << "invalid instruction (" << reason
                            << " \"" << specifiedText << "\")\n";
        }
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
        
    // AV = Argument Value
    enum class AVType : uint8_t
    {
        Textual, Numeric
    };
    enum class TextualAVType : uint8_t
    {
        VectorSpecified, FileOrNonePath, // File path or inexistent path
        FilePath, DirPath, DiskPath
    };
    struct ExtractedAV
    {
        std::string_view textualVal;
        uint64_t numericVal;
    };
    // Didn't use union here for simplicity
    struct AVInfo
    {
        std::string nameLowercase = "";
        AVType type = AVType::Textual;

        struct {
            TextualAVType type = TextualAVType::VectorSpecified;
            std::vector<std::string> expectedVals = {};
        } textual;
        struct {
            bool zeroIsUnacceptable = false, unitExpected = false;
        } numeric;
        
        std::string defaultVal = ""; // Also used for numeric values
        ExtractedAV* pExtracted = nullptr;
        
        AVInfo() = default;
        static AVInfo makeTextual(std::string_view nameLowercase, TextualAVType type,
                                            const std::vector<std::string>& expectedVals,
                                            std::string_view defaultVal, ExtractedAV* pExtracted)
        {
            AVInfo result;
            result.nameLowercase = nameLowercase;
            result.type = AVType::Textual;

            result.textual.type = type;
            result.textual.expectedVals = expectedVals;

            result.defaultVal = defaultVal;
            result.pExtracted = pExtracted;
            return result;
        }
        static AVInfo makeNumeric(std::string_view nameLowercase,
                                            bool zeroIsUnacceptable, bool unitExpected,
                                            std::string_view defaultValAsStr, ExtractedAV* pExtracted)
        {
            AVInfo result;
            result.nameLowercase = nameLowercase;
            result.type = AVType::Numeric;

            result.numeric.zeroIsUnacceptable = zeroIsUnacceptable;
            result.numeric.unitExpected = unitExpected;
            
            result.defaultVal = defaultValAsStr;
            result.pExtracted = pExtracted;
            return result;
        }
    };
    // AD = Argument Declarator (argument name or instruction name itself, before values)
    struct ADInfo
    {
        std::string nameLowercase = "";
        bool isRequired = false;
        size_t valCount = 0;
        AVInfo valInfos[2] = {};
        
        ADInfo() = default;
        ADInfo(std::string_view nameLowercase, bool isRequired, const std::vector<AVInfo>& valInfos)
        {
            if (valInfos.size() > 2)
            {
                std::cerr << "valInfos.size() > 2 given to DeclaratorInfo constructor in parseIns()"
                    << " (vector size = " << valInfos.size() << ")\n";
                exit(EXIT_FAILURE);
            }
            this->nameLowercase = nameLowercase;
            this->isRequired = isRequired;
            for (; valCount < valInfos.size(); valCount++)
                this->valInfos[valCount] = valInfos[valCount];
        }
    };
    // Validates, prints errors, and extracts all argument declarators and values
    // Instruction name (first argument declarator) must be compared to the desired string before call
    //   and must be the first value in declaratorInfos
    auto validatePrintExtractVals = [&](const std::vector<std::string_view>& segVecRef,
                                        const std::vector<ADInfo>& declaratorInfos)
                                                -> Utils::ErrorState
    {
        // Check empty declarator infos
        if (declaratorInfos.empty())
        {
            std::cerr << "empty declarator vector given to validatePrintExtractVals() in parseIns()\n";
            exit(EXIT_FAILURE);
        }

        // To catch duplicate declarators
        std::vector<bool> foundDecls(declaratorInfos.size(), false);
        for (size_t segI = 0; segI < segVecRef.size(); )
        {
            size_t currentDeclInfoI = SIZE_MAX;
            // Get current declarator index
            if (segI == 0) currentDeclInfoI = 0;
            else for (size_t declInfoI = 1; declInfoI < declaratorInfos.size(); declInfoI++)
            {
                if (Utils::compareLow(segVecRef[segI], declaratorInfos[declInfoI].nameLowercase))
                {
                    currentDeclInfoI = declInfoI;
                    break;
                }
            }
            // Check if segment didn't match any declarator
            if (currentDeclInfoI == SIZE_MAX)
            {
                printInvalidInsError("unexpected argument", segVecRef[segI]);
                return Utils::ErrorState::Failure;
            }
            // Check if the declarator is repeated
            const ADInfo& currentDeclInfo = declaratorInfos[currentDeclInfoI];
            if (foundDecls[currentDeclInfoI])
            {
                printInvalidInsError("repeated " +currentDeclInfo.nameLowercase +" argument", segVecRef[segI]);
                return Utils::ErrorState::Failure;
            }
            // Set current declarator index to found
            else foundDecls[currentDeclInfoI] = true;
            // Check if there are enough segments for the current declarator's operands
            if (segI +currentDeclInfo.valCount >= segVecRef.size())
            {
                // Declarator not printed if it's just the instruction name
                if (currentDeclInfo.valCount == 1)
                    printInvalidInsError("expected " +currentDeclInfo.valInfos[0].nameLowercase +" value",
                                            (currentDeclInfoI == 0)? "" : segVecRef[segI]);
                else
                    printInvalidInsError("expected " +currentDeclInfo.valInfos[0].nameLowercase
                                        +" and " +currentDeclInfo.valInfos[1].nameLowercase +" values",
                                            (currentDeclInfoI == 0)? "" : segVecRef[segI]);
                return Utils::ErrorState::Failure;
            }
            // Extract and validate current declarator values
            for (size_t valI = 0; valI < currentDeclInfo.valCount; valI++)
            {
                const AVInfo& valInfo = currentDeclInfo.valInfos[valI];
                const std::string_view valSeg = segVecRef[segI +1 +valI];
                // Textual value
                if (valInfo.type == AVType::Textual)
                {
                    bool valid = false;
                    if (valInfo.textual.type == TextualAVType::VectorSpecified)
                    {
                        // Any value allowed
                        if (valInfo.textual.expectedVals.empty()) valid = true;
                        // Specific values allowed
                        else for (const std::string& expectedVal : valInfo.textual.expectedVals)
                            if (Utils::compareLow(valSeg, expectedVal))
                            {
                                valid = true;
                                break;
                            }
                    }
                    // Path types
                    else
                    {
                        // ********************************************
                        // ********************************************
                        // Code to validate path types should be here
                        // ********************************************
                        // ********************************************
                        valid = true; 
                    }
                    // Print error or set extracted value
                    if (!valid)
                    {
                        printInvalidInsError("invalid " + valInfo.nameLowercase, valSeg);
                        return Utils::ErrorState::Failure;
                    }
                    else if (valInfo.pExtracted == nullptr)
                    {
                        std::cerr << "(textual) valInfo.pExtracted == nullptr given to"
                                    " validatePrintExtractVals() in parseIns()\n";
                        exit(EXIT_FAILURE);
                    }
                    else valInfo.pExtracted->textualVal = valSeg;
                }
                // Numeic value
                else if (valInfo.type == AVType::Numeric)
                {
                    uint64_t val = Utils::strToSize(valSeg, valInfo.numeric.zeroIsUnacceptable,
                                                    alwaysBinaryUnits, valInfo.numeric.unitExpected);
                    if (checkAndPrintErroneousSize(val, segVecRef[segI], valSeg,
                                                    valInfo.nameLowercase +" is too big"))
                        return Utils::ErrorState::Failure;
                    else if (valInfo.pExtracted == nullptr)
                    {
                        std::cerr << "(numeric) valInfo.pExtracted == nullptr given to"
                                    " validatePrintExtractVals() in parseIns()\n";
                        exit(EXIT_FAILURE);
                    }
                    else valInfo.pExtracted->numericVal = val;
                }
            }
            // Progress main loop
            segI += (1 +currentDeclInfo.valCount);
        }

        // Check missing required declarators and set default values for missing optional ones
        for (size_t declInfoI = 1; declInfoI < declaratorInfos.size(); declInfoI++)
        {
            if (foundDecls[declInfoI]) continue;
            // Error for missing required declarators
            if (declaratorInfos[declInfoI].isRequired)
            {
                printInvalidInsError("expected " + declaratorInfos[declInfoI].nameLowercase + " argument", "");
                return Utils::ErrorState::Failure;
            }
            // Set default values for missing optional declarators
            for (size_t valI = 0; valI < declaratorInfos[declInfoI].valCount; valI++)
            {
                const AVInfo& valInfo = declaratorInfos[declInfoI].valInfos[valI];
                if (!valInfo.pExtracted) continue;
                // Textual value
                if (valInfo.type == AVType::Textual)
                {
                    valInfo.pExtracted->textualVal = valInfo.defaultVal;
                }
                // Numeric value
                else if (valInfo.type == AVType::Numeric)
                {
                    uint64_t result = Utils::strToSize(valInfo.defaultVal, valInfo.numeric.zeroIsUnacceptable,
                                                        alwaysBinaryUnits, valInfo.numeric.unitExpected);
                    if (result >= (uint64_t)Utils::SizeSig::LEAST_ERROR)
                    {
                        std::cerr << "(numeric) invalid valInfo.defaultVal given to"
                                    " validatePrintExtractVals() in parseIns()\n";
                        exit(EXIT_FAILURE);
                    }
                    else valInfo.pExtracted->numericVal = result;
                }
            }
        }

        return Utils::ErrorState::Success;
    };

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
    {
        resultIns.type = InsType::InternalSkip;
    }
    // help
    else if (Utils::compareLow(segments[0], "help"))
    {
        if (validatePrintExtractVals(segments, {
            ADInfo("help", true, {})
        }) != Utils::ErrorState::Success) error = true;
        else
        {
            Utils::printHelp();
            resultIns.type = InsType::InternalSkip;
        }
    }
    
    // allyes
    else if (Utils::compareLow(segments[0], "allyes"))
    {
        if (validatePrintExtractVals(segments, {
            ADInfo("allyes", true, {})
        }) != Utils::ErrorState::Success) error = true;
        else
            resultIns = Ins(InsType::SetYes, makeSwitchInfo(true));
    }
    // manyes
    else if (Utils::compareLow(segments[0], "manyes"))
    {
        if (validatePrintExtractVals(segments, {
            ADInfo("manyes", true, {})
        }) != Utils::ErrorState::Success) error = true;
        else
            resultIns = Ins(InsType::SetYes, makeSwitchInfo(false));
    }
    
    // binary
    else if (Utils::compareLow(segments[0], "binary"))
    {
        if (validatePrintExtractVals(segments, {
            ADInfo("binary", true, {})
        }) != Utils::ErrorState::Success) error = true;
        else
            resultIns = Ins(InsType::SetBinary, makeSwitchInfo(true));
    }
    // decimal
    else if (Utils::compareLow(segments[0], "decimal"))
    {
        if (validatePrintExtractVals(segments, {
            ADInfo("decimal", true, {})
        }) != Utils::ErrorState::Success) error = true;
        else
            resultIns = Ins(InsType::SetBinary, makeSwitchInfo(false));
    }

    // openvd
    else if (Utils::compareLow(segments[0], "openvd"))
    {
        // e.g. openvd x.img size 32gib sectsize 4096B
        ExtractedAV path, size, sectorSize;
        if (validatePrintExtractVals(segments, {
            ADInfo("openvd", true, {
                AVInfo::makeTextual("file name", TextualAVType::FileOrNonePath, {}, "", &path)
            }),
            ADInfo("size", false, {
                AVInfo::makeNumeric("size", true, true, "64MiB", &size)
            }),
            ADInfo("sectsize", false, {
                AVInfo::makeNumeric("sector size", true, true, "512B", &sectorSize)
            })
        }) != Utils::ErrorState::Success) error = true;
        else
        {
            resultIns.type = InsType::OpenDisk;
            resultIns.info.openDisk.isReal = false;
            resultIns.info.openDisk.setPath(&resultIns, path.textualVal);
            resultIns.info.openDisk.size = size.numericVal;
            resultIns.info.openDisk.sectorSize = sectorSize.numericVal;
            resultIns.info.openDisk.physicalSectorSize = sectorSize.numericVal;
        }
    }
    // scheme
    //else if (Utils::compareLow(segments[0], "scheme"))
    //{
    //}
    
    // save
    else if (Utils::compareLow(segments[0], "save"))
    {
        if (validatePrintExtractVals(segments, {
            ADInfo("save", true, {})
        }) != Utils::ErrorState::Success) error = true;
        else
            resultIns = Ins(InsType::Save, InsInfo());
    }
    // stop, exit, quit
    else if (Utils::compareLow(segments[0], "stop")
            || Utils::compareLow(segments[0], "exit")
            || Utils::compareLow(segments[0], "quit"))
    {
        if (segments.size() > 1)
        {
            printInvalidInsError("unexpected arguments", "");
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

// Validates instructions file and returns instructions vector (empty if contains errors)
std::vector<Inss::Ins> parseFile(const char* path, bool alwaysBinaryUnits)
{
    using namespace Inss;
    // Returns:
    //   1. Clean string view (no comments or line-continue character)
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
            // Hashtag may be part of instructions so gotta ensure nothing is stuck behind it like "something#"
            {
                lineView.remove_suffix(i +1);
                break;
            }
        }
        // Check for and remove line-continue character
        bool continueNextLine = false;
        for (size_t i = lineView.size() -1; i != SIZE_MAX; i--)
        {
            if (isspace((unsigned char)lineView[i])) continue;
            else if (lineView[i] != '\\') break; // Normal character (not space nor the symbol)
            else // line-continue character "\"
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
        //   then the loop stops even if the last line has a redundant line-continue character "\"
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
        std::pair<Ins, Utils::ErrorState> parseResult = parseIns(insString, alwaysBinaryUnits, false);
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
    for (size_t i = 1; i < argC; i++)
    {
        // Instruction separator for direct argument passing
        if (!strcmp(argV[i], "-"))
        {
            // Add argument segments with padding together
            // This is kinda dumb because parseIns cuts it back again but idc
            insString = ""; // Clear previous string
            for (size_t j = 1; j < i; j++)
            {
                insString.append(argV[j]);
                insString += ' ';
            }
            // Parse
            Ins ins = NONE_INS;
            Utils::ErrorState errorState = Utils::ErrorState::Failure;
            std::pair<Ins, Utils::ErrorState> parseResult = parseIns(insString, alwaysBinaryUnits, false);
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
    return parseIns(line, alwaysBinaryUnits, true).first;
}
