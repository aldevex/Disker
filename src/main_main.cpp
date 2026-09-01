#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include "structs/generic.hpp"
#include "structs/mbr.hpp"
//#include "structs/gpt.hpp"
//#include "structs/fat32.hpp"
#include "algos/cmd.hpp"

// Only mainMain() and applyIns() may access this directly
static struct _StateStruct
{
friend int mainMain(int argC, char** argV);
friend void applyIns(const Cmd::Ins& ins);
private:
    bool allYes = false;
    bool binaryUnits = true;

    Generic::Disk disk;

    /*
    //Generic::Scheme schemeType;
    //MBRns::MBRData MBR;
    //GPTns::GPTCollection GPTColl;
    
    struct {
        // Stuff
    } GPT;
    
    struct {
        // More stuff
    } partitionData;
    */

public:
    // Struct name and constructor required to find field constructors' issues
    //   because Mr. Stroustrup has made a great programming language
    _StateStruct() = default;
} state;

void applyIns(const Cmd::Ins& ins)
{
    using namespace Cmd;

    switch (ins.type)
    {
    case InsType::None:
        break;
    case InsType::SetYes:
        state.allYes = ins.info.switchValue;
        break;
    case InsType::SetBinary:
        state.binaryUnits = ins.info.switchValue;
        break;
    case InsType::OpenDisk:
        // Or make an open disk function ig better
        state.disk = Generic::Disk(ins.info.openDisk.getPath(&ins.info), ins.info.openDisk.isReal,
                                    ins.info.openDisk.size, ins.info.openDisk.sectorSize,
                                    ins.info.openDisk.physicalSectorSize,
                                    Utils::strToSize("1MiB", true, true, true));
        break;
    case InsType::Exit:
        exit(EXIT_SUCCESS);
        break;
    default:
        std::cerr << "unprogrammed instruction type given to applyIns (" << (uint16_t)ins.type << ")\n";
        exit(EXIT_FAILURE);
        break;
    }
}

int mainMain(int argC, char** argV)
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
        std::vector<Cmd::Ins> instructions = Cmd::parseFile(cmdFilePath, state.binaryUnits);
        for (const Cmd::Ins& ins : instructions)
            applyIns(ins);
    }
    // Commandline
    else
    {
        std::cout << "type \"help\" for help | type \"quit\" to exit the program" << std::endl;
        while (true)
            applyIns(Cmd::parseTerminal(state.binaryUnits));
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
