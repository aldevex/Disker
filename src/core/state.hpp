// core.hpp include either in tui_main.cpp or nappgui.cpp you mustn't compile both
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include "../algos/utils.hpp"
#include "../structs/generic.hpp"
//#include "../structs/mbr.hpp"
//#include "structs/gpt.hpp"
//#include "structs/fat32.hpp"
#include "../algos/inss.hpp"

// Only TUI/GUI main functions and applyIns() may access this directly
static struct _StateStruct
{
friend int TUIMain(int argC, char** argV);
friend int GUIMain();
friend void applyIns(const Inss::Ins& ins);
private:
    bool allYes = false;
    bool alwaysBinaryUnits = true;

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

void applyIns(const Inss::Ins& ins)
{
    using namespace Inss;
    switch (ins.type)
    {
    case InsType::None:
        break;
    case InsType::SetYes:
        state.allYes = ins.info.switchValue;
        break;
    case InsType::SetBinary:
        state.alwaysBinaryUnits = ins.info.switchValue;
        break;
    case InsType::OpenDisk:
        // Or make an open disk function ig better
        state.disk = Generic::Disk(ins.info.openDisk.getPath(&ins), ins.info.openDisk.isReal,
                                    ins.info.openDisk.size, ins.info.openDisk.sectorSize,
                                    ins.info.openDisk.physicalSectorSize,
                                    Utils::strToSize("1MiB", true, true, true));
        break;
    case InsType::Save:
        if (!state.disk.isRealDisk)
        {
            std::vector<uint8_t> hugeImgBuffer(state.disk.size, 0);
            Utils::writeFile(state.disk.path.c_str(), hugeImgBuffer.data(), hugeImgBuffer.size());
        }
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
