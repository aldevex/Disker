// inss.hpp (instructions)
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
#include "../structs/generic.hpp"
// Instructions
namespace Inss {

// Instruction type
enum class InsType : uint16_t
{
    None,
    InternalSkip, // Should be converted to None before sending outside parsing code

    SetYes, // allyes/manyes
    SetBinary, // binary/decimal instructions (for gb/mb/etc. units)

    OpenDisk, // openvd/openphd
    SetScheme, // Set disk scheme (MBR/GDT) and partition count
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



class Ins;
union InsInfo
{
    bool switchValue;

    struct _OpenDiskStruct
    {
        uint64_t size,
                sectorSize,
                physicalSectorSize;
        bool isReal;
        void setPath(Ins* pParentIns, std::string_view pathView);
        std::string_view getPath(const Ins* pParentIns) const;
    } openDisk;

    struct
    {
        Generic::Scheme scheme;
        uint64_t partitionCount;
    } setScheme;

    InsInfo()
        { memset(this, 0, sizeof(InsInfo)); }
};

// Instruction
class Ins
{
friend union InsInfo;
private:
    std::string textBuffer1 = "", textBuffer2 = "";
public:
    InsType type = InsType::None;
    InsInfo info;
    
    Ins() = default;
    Ins(InsType type, InsInfo info)
        : type(type), info(info)
    {}
};

void InsInfo::_OpenDiskStruct::setPath(Ins* pParentIns, std::string_view pathView)
    { pParentIns->textBuffer1 = pathView; }
std::string_view InsInfo::_OpenDiskStruct::getPath(const Ins* pParentIns) const
    { return pParentIns->textBuffer1; }
    


const Ins NONE_INS = Ins();

}
