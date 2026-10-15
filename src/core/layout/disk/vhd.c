#include "./vhd.h"

// Spec https://www.microsoft.com/en-us/download/details.aspx?id=23850

// Spec says Microsoft was stupid so they used a 511 byte footer at the end instead of 512
//   so you just have to deal with it if you get a 511 byte footer at the end

// Also VHD is BE so you must swap bytes on file R/W

// VHD sector size is always 512B because Microsoft evil

typedef struct ATTRIB_PACKED CHS
{
    uint16_t cylinder;
    uint8_t heads;
    uint8_t spt; // Sectors per track
} CHS;

typedef struct ATTRIB_PACKED Footer
{
    uint8_t cookie[8];
    uint32_t features;
    uint32_t fileFormatVersion;
    uint64_t dataOffset; // Byte offset to dynamic disk header
    uint32_t timeStamp;
    uint8_t creatorApp[4];
    uint32_t creatorVersion;
    uint8_t creatorHostOS[4];

    uint64_t ogSize;
    uint64_t currentSize;

    CHS diskGeometry;
    uint32_t diskType;
    
    uint32_t checksum;
    uint8_t uniqueID[16];
    uint8_t savedState;

    // 427 reserved bytes in file
} Footer;

typedef struct ATTRIB_PACKED ParentLocatorEntry
{
    uint32_t platformCode;
    uint32_t platformDataSpace;
    uint32_t platformDataLength;
    uint32_t reserved;
    uint64_t platformDataOffset;
} ParentLocatorEntry;

typedef struct ATTRIB_PACKED DynamicDiskHeader
{
    uint8_t cookie[8];
    uint64_t dataOffset; // Byte offset to next structure (unused, set to UINT**32**_MAX)
    uint64_t tableOffset; // Byte offset to BAT (block allocation table)
    uint32_t headerVersion;

    uint32_t maxTableEntries; // Max BAT entries | Should be (disk size / block size) to fit full expanded dynamic disk
    uint32_t blockSize; // Dynamic expansion block size in BYTES | Default 2MiB
    uint32_t checksum;

    uint8_t parentUniqueID[16]; // For differencing VHD
    uint32_t parentTimeStamp;
    uint32_t reserved;

    uint16_t parentUnicodeName[256]; // UTF-16BE
    ParentLocatorEntry parentLocatorEntries[8];

    // 256 reserved bytes in file
} DynamicDiskHeader;

// The block allocation table (BAT) is just an array of 32-bit sector offsets relatove to file start
// Unused entries set to UINT32_MAX (0xFF,FF,FF,FF)
// Sectors per block must always be a power of two

/* Fixed VHD layout:
- Raw data
- Footer (may be 511 or 512 bytes)
*/

/* Dynamic (includes differencing) VHD layout:
- Mirror of footer
- Dynamic VHD header (spec says "Sparse Drive Header", maybe forgot to rename to "Dynamic")
- BAT
- Data blocks pointed-to by BAT
- Footer (may be 511 or 512 bytes)
*/

// Constants

// static const ASCII8_t* COOKIE_MS = "conectix";
static const ASCII8_t* COOKIE_DR = "Disker  ";

#define FEATURE_TEMPORARY 0x1
#define FEATURE_RESERVED 0x2 // Must always be set in footer features

#define FILE_FORMAT_V10 0x00010000 // v1.0

// static const ASCII8_t* CREATOR_VPC = "vpc "; // Microsoft virtual PC
// static const ASCII8_t* CREATOR_VS = "vs  "; // Microsoft virtual server
static const ASCII8_t* CREATOR_DR = "Dskr";

static const ASCII8_t* HOST_WIN = "Wi2k";
// static const ASCII8_t* HOST_MAC = "Mac ";

#define TYPE_NONE          0
#define TYPE_DEPRECATED1   1
#define TYPE_FIXED         2
#define TYPE_DYNAMIC       3
#define TYPE_DIFFERENCING  4
#define TYPE_DEPRECATED2   5
#define TYPE_DEPRECATED3   6

es_t ldvhd_create(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo)
{
    
}

// es_t ldvhd_close(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);

// void ldvhd_free(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo);
