#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <random>
#include "../utils.hpp"
namespace GPTns {
#pragma pack(push, 1)

// Basic GPT disk architecture:
// Sector 0: Protective MBR (contains 1 specifica partiton entry)
// Sector 1: Primary GPT Header
// Sector 2-33: GPT Partition Entry Array
// Sector 34-(N-34): Usable LBA space:
//   Sector 34-2047: Alignment Gap
//   Sector 2048+: ESP (Possible Stage 2 for BIOS environment)
//   Sector (After ESP End)+: OS Partition
// Sector (N-33)-(N-2): Backup GPT Partition Entry Array
// Sector (N-1): Backup GPT Header

// Microsoft GUID (UUIDv4 generation technique all random bits)
// Fields are normal little endian except data4,
//   unlike normal UUID which is completely big endian
// Exits with failure if you give a constructor an invalid GUID, use validate() beforehand
struct MSGUID
{
    uint32_t data1 = 0;
    uint16_t data2 = 0;
    uint16_t data3 = 0; // 4 version bits high 12-15
    uint8_t data4[8] = {}; // 2-4 (variable length) variant bits high 4-7 (variable length) of data4[0]
    
    MSGUID() = delete;
    static MSGUID rand()
    {
        // Old code when i used UUIDv7 technique (minus big endianness)
        // Don't delete it plz
        /*
        // UUIDv7 struct is like:
        // struct
        // {
        //     uint8_t timestamp[6] = UNDEFINED_ARRAY; // 48 bits
        //     uint16_t ver_rand_a;
        //     uint64_t var_rand_b;
        // } UUIDv7;
        uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        uint64_t random1 = 0, random2 = 0;
        {
            thread_local std::random_device randomDevice;
            thread_local std::mt19937_64 generator(randomDevice());
            thread_local std::uniform_int_distribution<uint64_t> distributor;
            random1 = distributor(generator);
            random2 = distributor(generator);
        }
        // 48 bit timestamp
        data1 = (uint32_t)(time >> 16); // Top 32 bits
        data2 = (uint16_t)(time & 0xFFFF); // Bottom 16 bits
        // Random bits
        data3 = (uint16_t)(random1);
        data4[0] = (uint8_t)(random2 >> 56);
        data4[1] = (uint8_t)(random2 >> 48);
        data4[2] = (uint8_t)(random2 >> 40);
        data4[3] = (uint8_t)(random2 >> 32);
        data4[4] = (uint8_t)(random2 >> 24);
        data4[5] = (uint8_t)(random2 >> 16);
        data4[6] = (uint8_t)(random2 >> 8);
        data4[7] = (uint8_t)(random2);
        // Set UUID version 7
        data3 &= 0x0FFF;
        data3 |= (0x7 << 12);
        // Set UUID variant 1 (its pattern is 10b = 2)
        data4[0] &= 0b00111111;
        data4[0] |= (0b10 << 6);
        */

        uint32_t data1 = 0;
        uint16_t data2 = 0;
        uint16_t data3 = 0;
        uint8_t data4[8] = {};
        // Set random bits
        uint64_t random1 = 0, random2 = 0;
        {
            thread_local std::random_device randomDevice;
            thread_local std::mt19937_64 generator(randomDevice());
            thread_local std::uniform_int_distribution<uint64_t> distributor;
            random1 = distributor(generator);
            random2 = distributor(generator);
        }
        data1 = (uint32_t)(random1 & 0xFFFF'FFFF);
        data2 = (uint16_t)(random1 >> 32);
        data3 = (uint16_t)(random1 >> 48);
        *((uint64_t*)data4) = random2;
        // Set UUID version 4
        data3 &= 0x0FFF;
        data3 |= (0x4 << 12);
        // Set UUID variant 1 (its pattern is 10b = 2)
        data4[0] &= 0b00111111;
        data4[0] |= (0b10 << 6);

        return MSGUID(data1, data2, data3, data4);
    }
    MSGUID(const char* str)
        : MSGUID(std::string(str))
    {}
    MSGUID(const std::string& str)
    {
        if (!validate(str))
        {
            perror("invalid GUID string @ MSGUID constructor");
            exit(EXIT_FAILURE);
        }
        // Example UUIDv7: 00000000-0000-7000-8000-000000000000
        // Example UUIDv4: 00000000-0000-4000-8000-000000000000
        // First segment (8 digits = 4 bytes)
        data1 = parseHex<uint32_t>(str.substr(0, 8));
        // Second segment (4 digits = 2 bytes)
        data2 = parseHex<uint16_t>(str.substr(9, 4));
        // Third segment (4 digits = 2 bytes)
        data3 = parseHex<uint16_t>(str.substr(14, 4));
        // Fourth segment (4 digits = 2 bytes)
        data4[0] = parseHex<uint8_t>(str.substr(19, 2));
        data4[1] = parseHex<uint8_t>(str.substr(21, 2));
        // Fifth segment (12 digits = 6 bytes)
        for (size_t i = 0; i < 6; i++)
            data4[2 +i] = parseHex<uint8_t>(str.substr(24 + (i *2), 2));
    }

    // Returns true if given GUID string is valid
    static bool validate(std::string_view view)
    {
        if (view.length() != 36 || view[8] != '-' || view[13] != '-' || view[18] != '-' || view[23] != '-')
            return false;
    }

    bool cmp(const MSGUID& other) const
    {
        return other.data1 == data1 && other.data2 == data2 && other.data3 == data3
            && *((uint64_t*)other.data4) == *((uint64_t*)data4);
    }

    std::string toStr() const
    {
        std::string str(36, '?');
        std::snprintf(str.data(), str.length() +1,
                    "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                    data1, data2, data3, data4[0], data4[1],
                    data4[2], data4[3], data4[4],
                    data4[5], data4[6], data4[7]);
        return str;
    }

private:
    MSGUID(uint32_t data1, uint16_t data2, uint16_t data3, uint8_t* data4)
    {
        this->data1 = data1;
        this->data2 = data2;
        this->data3 = data3;
        *((uint64_t*)this->data4) = *((uint64_t*)data4);
    }

    template <typename T>
    static T parseHex(std::string_view hex)
    {
        auto getDigit = [](char c) -> T
        {
            if (c >= '0' && c <= '9') return c -'0';
            else if (c >= 'a' && c <= 'f') return c -'a' +10;
            else if (c >= 'A' && c <= 'F') return c -'A' +10;
            else
            {
                perror((std::string("invalid hexadecimal character \'") +c +"\' @MSGUID::parseHex").c_str());
                exit(EXIT_FAILURE);
            }
        };
        T result = 0;
        for (char c : hex) result = (result << 4) | getDigit(c);
        return result;
    }
};

MSGUID ZERO_GUID = "00000000-0000-0000-0000-000000000000";
// The no-param constructor generates a random GUID so it cannot be used for default value
MSGUID UNDEFINED_GUID = ZERO_GUID;

// Enum larp for partition type GUIDs
const struct
{
    MSGUID ESP = "C12A7328-F81F-11D2-BA4B-00A0C93EC93B";
    MSGUID FAT32 = "E7C63C1E-8EDF-11D2-A2C9-00A0C93EC93B";
} PartitionTypeGUID;

enum class PEAttributeFLag : uint64_t
{
    SystemPartition = (1ULL << 0), // Critical system partition (Bit 0)
    IgnoreByEFI = (1ULL << 1), // Firmware skips booting (Bit 1)
    LegacyBootable = (1ULL << 2) // Legacy BIOS active flag (Bit 2)
};

struct PartitionEntry
{
    MSGUID partitionTypeGUID = UNDEFINED_GUID; // OS-dependant, can be anything
    MSGUID uniquePartitionGUID = UNDEFINED_GUID;
    uint64_t firstLBA;
    uint64_t lastLBA;
    uint64_t attributeFlags;
    uint16_t partitionName16[36]; // Null-terminated
    // Spec allows 36 characters without null but that's dangerous!

    PartitionEntry() = default;
    PartitionEntry(const char16_t* partitionName_UTF16_max35, MSGUID partitionGUID, MSGUID partitionTypeGUID,
                    uint64_t firstLBA, uint64_t lastLBA, uint64_t attributeFlags)
    {
        this->partitionTypeGUID = partitionTypeGUID;
        this->uniquePartitionGUID = partitionGUID;
        this->firstLBA = firstLBA;
        this->lastLBA = lastLBA;
        this->attributeFlags = attributeFlags;
        for (size_t i = 0; i < 36; i++)
        {
            if (i == 35 || partitionName_UTF16_max35[i] == '\0')
            {
                for (; i < 36; i++) this->partitionName16[i] = u'\0';
                break;
            }
            else this->partitionName16[i] = partitionName_UTF16_max35[i];
        }
    }
};

struct Header
{
    uint64_t signature; // Must be set to "EFI PART"
    uint32_t revision = 0x10000; // GPT version 1.0
    uint32_t headerSize = 92;
    uint32_t headerCRC32; // Must be set to CRC32 checksum (HAS TO BE 0 INITIALLY)
    uint32_t reserved = 0;
    uint64_t myLBA; // Must be set to header LBA
    uint64_t alternateLBA; // Must be set to header opposite LBA (backup for og, og for backup)
    uint64_t firstUsableLBA; // Must be set
    uint64_t lastUsableLBA; // Must be set to (Total -34)
    MSGUID diskGUID = UNDEFINED_GUID; // Must be set to a random GUID (must be identical in backup header)
    uint64_t partitionEntryLBA; // Must be set to parition entry array start
    uint32_t numberOfPartitionEntries; // Commonly 128 but can be anything
    uint32_t sizeOfPartitionEntry = 128; // Can actually be >128 but that's for niche custom usage
    uint32_t partitionEntryArrayCRC32; // Must be set to partiton array CRC32 checksum
    uint8_t reservedBlock[420] = {}; // HEEEEEEEEEELP THIS IS SUPPOSED TO BE TILL END OF SECTOR
    // WHAT IF THE SECTOR IS 4096 BYTES THEN IT WILL BE MORE THAN 420 ZERO BYTES

    Header(uint64_t totalSectors, bool backupHeader,
                PartitionEntry* partitionEntryArray, MSGUID diskGUID)
    {
        memcpy(&signature, "EFI PART", 8);

        myLBA = (!backupHeader)? 1  : totalSectors -1;
        alternateLBA = (!backupHeader)? totalSectors -1  : 1;

        firstUsableLBA = ; // Must be set
        lastUsableLBA = totalSectors -34; // nooooooooo 34 wont always work !!!!!!!!!!!!!!!!!!!!!!!!!!

        this->diskGUID = diskGUID;
        partitionEntryLBA = (!backupHeader)? 2  : totalSectors -33;
        numberOfPartitionEntries = ;

        partitionEntryArrayCRC32 = Utils::crc32(partitionEntryArray, 128*128);
        headerCRC32 = 0;
        headerCRC32 = Utils::crc32(this, 92);
    }
};

#pragma pack(pop)    
}
