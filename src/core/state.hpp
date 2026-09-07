// core.hpp include either in tui_main.cpp or nappgui.cpp you mustn't compile both
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include "../utils/ssutils.hpp"
#include "../structs/disk.hpp"
//#include "../structs/mbr.hpp"
//#include "structs/gpt.hpp"
//#include "structs/fat32.hpp"
#include "./inss.hpp"

// Only TUI/GUI main functions and applyIns() may access this directly
static struct _StateStruct
{
friend int TUIMain(int argC, char** argV);
friend int GUIMain();
friend void applyIns(const Inss::Ins& ins);
private:
    bool allYes = false;
    bool alwaysBinaryUnits = true;

    Disk::Disk disk;

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
        if (!ins.info.openDisk.isReal)
        {
            if (Utils::getPathType(ins.info.openDisk.getPath(&ins)) == Utils::PathType::File)
            {
                // .data() is ok here because view points to a full std::string
                Utils::ReadImageInfo readInfo = Utils::readImage(ins.info.openDisk.getPath(&ins).data());
                if (readInfo.errorState != Utils::ErrorState::Success) return;
                std::cout << "Image file \"" << ins.info.openDisk.getPath(&ins) << "\":"
                << "\n  - Size: " << readInfo.disk.size << " bytes"
                << "\n  - Scheme: ";
                if (readInfo.scheme == Disk::Scheme::MBR) std::cout << "MBR";
                else std::cout << "GPT";
                std::cout << "\n";
            }
            state.disk = Disk::Disk(ins.info.openDisk.getPath(&ins), ins.info.openDisk.isReal,
                                    ins.info.openDisk.size, ins.info.openDisk.sectorSize,
                                    ins.info.openDisk.physicalSectorSize,
                                    Utils::strToSize("1MiB", true, true, true));
        }
        break;
    case InsType::Save:
        if (!state.disk.isRealDisk)
        {
            if (!state.allYes
            && Utils::getPathType(state.disk.path) == Utils::PathType::File)
            {
                std::cout << "Are you sure you want to overwrite \"" << state.disk.path << "\"?\n"
                << "yes/no" << std::endl;
                // Get answer
                std::string line;
                std::getline(std::cin, line);
                // Remove new line characters
                if (!line.empty() && line.back() == '\n') line.pop_back();
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!Utils::compareLow(line, "yes")) return;
            }
            Utils::writeImage(state.disk.path.c_str(), state.disk.size, {});
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
