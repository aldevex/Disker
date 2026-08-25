#include <cstdio>
#include <string>
#include <iostream>
#include "structs/mbr.hpp"
//#include "structs/gpt.hpp"
//#include "structs/fat32.hpp"

enum class Format
{
    MBR, GPT
};

struct {
    bool allYes = false;
    bool binaryUnits = true;

    struct {
        std::string name = "";
        bool isRealDisk = true; // True default for safety
        size_t size = 0, sectorCount = 0, sectorSize = 0;
        Format format = Format::MBR;
    } disk;

    struct {
        MBRns::MBRData data;
    } MBR;

    struct {
        // Stuff
    } GPT;
    
    struct {
        // More stuff
    } partitionData;
} globalState;

#include "algos/cmd.hpp"

void applyIns(const Cmd::Ins& ins)
{
    using namespace Cmd;
    switch (ins.type)
    {
    case InsType::None:
        break;
    case InsType::Skip:
        std::cerr << "Skip instruction type given to applyIns, it should be sent as None\n";
        exit(EXIT_FAILURE);
        break;
    case InsType::AllYes:
        globalState.allYes = true;
        break;
    case InsType::ManYes:
        globalState.allYes = false;
        break;
    case InsType::Exit:
        exit(EXIT_SUCCESS);
        break;
    default:
        std::cerr << "unsupported instruction type given to applyIns (" << (uint16_t)ins.type << ")\n";
        exit(EXIT_FAILURE);
        break;
    }
}

int main(int argC, char** argV)
{
    const char* cmdFilePath = nullptr;
    if (argC == 3)
    {
        if (strcmp(argV[1], "do")) argC = INT_MAX; // For next if statement failure
        else cmdFilePath = argV[2];
    }
    // No "else", just make sure expected amount of args is checked
    if (argC != 1 && argC != 3)
    {
        std::cerr << "invalid arguments. expected nothing or \"do <FILENAME>\"\n";
        exit(EXIT_FAILURE);
    }
    
    // File
    if (cmdFilePath != nullptr)
    {
        std::vector<Cmd::Ins> instructions = Cmd::parseFile(cmdFilePath);
        for (const Cmd::Ins& ins : instructions)
            applyIns(ins);
    }
    // Commandline
    else
    {
        std::cout << "type \"help\" for help | type \"quit\" to exit the program" << std::endl;
        while (true)
            applyIns(Cmd::parseTerminal());
    }

    /*
    uint64_t totalSectors = 80;
    uint32_t totalSectors32 = (uint32_t)totalSectors;
    if (uint64_t(totalSectors32) != totalSectors) totalSectors32 = 0xFFFFFFFF;

    uint8_t MBRCode[446] = {};
    MBR::PartitionEntry MBRPartitionTable[4];
    MBRPartitionTable[0] = MBR::PartitionEntry(false, MBR::PartitionType::GPTProtectiveMBR,
                                                    1, totalSectors32, 0, 2, 0, 0xFF, 0xFF, 0xFF);
    MBR::MBR protectiveMBR(MBRCode, MBRPartitionTable);

    GPT::PartitionEntry partitionEntries[128] = {};
    partitionEntries[0] = GPT::PartitionEntry(u"ESP", RANDOM_GUID, ZERO_GUID,
                                2048, 2048 +8, (uint64_t)GPT::PEAttributeFLag::SystemPartition);
    partitionEntries[1]= GPT::PartitionEntry(u"OS", RANDOM_GUID, ZERO_GUID,
                                2048 +9, 2048 +9 +16, (uint64_t)GPT::PEAttributeFLag::SystemPartition);

    const GPT::MSGUIDv7 headerGUID = RANDOM_GUID;
    GPT::Header header(80, false, partitionEntries, headerGUID);
    GPT::Header backupHeader(80, true, partitionEntries, headerGUID);

    printf("%s\n", headerGUID.toStr().c_str());
    */
    return EXIT_SUCCESS;
}
