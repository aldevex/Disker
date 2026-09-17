#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include "../utils/generic.h"
#include "../scheme/scheme.h"
#include "../fs/fs.h"
#include "../diskinfo.h"
#include "./instructions.h"

typedef struct _StateStruct
{
    bool allYes;
    bool alwaysBinaryUnits;
    DiskInfo ogDiskInfo, editedDiskInfo;
} _StateStruct;
// No one should access this except applyIns(), tuiMain(), and guiMain()
static _StateStruct state = {
    .allYes = false,
    .alwaysBinaryUnits = true,
    // Setting this makes an error because struct inside struct or something
    //.diskInfo = {0}
};

utils_ErrorState applyIns(Ins* pIns)
{
    switch (pIns->type)
    {
    case INSTYPE_NONE:
        break;
    case INSTYPE_SET_YES:
        state.allYes = pIns->info.switchValue;
        break;
    case INSTYPE_SET_BINARY:
        state.alwaysBinaryUnits = pIns->info.switchValue;
        break;
    case INSTYPE_OPEN_DISK:
        // utils_ErrorState closeState = UTILS_ERRORSTATE_SUCCESS;
        // if (state.ogDiskInfo.handle != UTILS_HANDLE_NONE)
        // {
        //     if (state.ogDiskInfo.geometry.type != GEO_TYPE_RAW_DISK)
        //         closeState = closeImage(&state.ogDiskInfo);
        // }
        // if (closeState != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
        // bool createdNewFile = false;
        // if (state.ogDiskInfo.geometry.type != GEO_TYPE_RAW_DISK)
        // {
        //     utils_ErrorState readState = openLockReadImage(&state.ogDiskInfo,
        //             insInfoOpenDiskGetPath(&pIns->info), &createdNewFile);
        //     if (readState != UTILS_ERRORSTATE_SUCCESS) return UTILS_ERRORSTATE_FAILURE;
        // }
        // const char* schemeText = NULL;
        // if (state.ogDiskInfo.scheme.type == SCHEME_TYPE_MBR) schemeText = "MBR";
        //                                                 else schemeText = "GPT";
        // if (!createdNewFile) printf(
        //     "Opened disk image \"%s\":\n"
        //     "  - Size: %llu bytes\n"
        //     "  - Sector size: %llu\n"
        //     "  - Alignment: %llu\n"
        //     "  - Scheme: %s\n"
        //     "\n", string8NT(&state.ogDiskInfo.path),
        //     state.ogDiskInfo.geometry.data.raw.size,
        //     state.ogDiskInfo.geometry.data.raw.sectorSize,
        //     state.ogDiskInfo.geometry.data.raw.alignment,
        //     schemeText
        // );
        break;
    case INSTYPE_SAVE:
        // if (!state.disk.isRealDisk)
        // {
        //     if (!state.allYes
        //     && Utils::getPathType(state.disk.path) == Utils::PathType::File)
        //     {
        //         std::cout << "Are you sure you want to overwrite \"" << state.disk.path << "\"?\n"
        //         << "yes/no" << std::endl;
        //         // Get answer
        //         std::string line;
        //         std::getline(std::cin, line);
        //         // Remove new line characters
        //         if (!line.empty() && line.back() == '\n') line.pop_back();
        //         if (!line.empty() && line.back() == '\r') line.pop_back();
        //         if (!Utils::compareLow(line, "yes")) return;
        //     }
        //     Utils::writeImage(state.disk.path.c_str(), state.disk.size, {});
        // }
        break;
    case INSTYPE_EXIT:
        exit(EXIT_SUCCESS);
        break;
    default:
        fprintf(stderr, "unprogrammed instruction type given to %s (pIns->type = %u)\n",
                        __func__, pIns->type);
        exit(EXIT_FAILURE);
        break;
    }
    return UTILS_ERRORSTATE_SUCCESS;
}
