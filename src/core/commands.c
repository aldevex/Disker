#include "./commands.h"



utils_ErrorState applyEditDisk(ApplyCmdInfo* pInfo)
{
    CmdInfoEditDisk cmdInfo = pInfo->pCmd->info.editDisk;
    DiskInfo diskInfo = *pInfo->pDiskInfo;
    if (cmdInfo.size != UTILS_SIZESIG_NO_NUMBER
    && cmdInfo.size != diskInfo.geometry.data.raw.size)
    {
        // Extend file size
        if (cmdInfo.size > diskInfo.geometry.data.raw.size)
        {
            //utils_extendFile();
        }
        // Shrink file size
        else
        {

        }
    }
    if (cmdInfo.sectorSize != UTILS_SIZESIG_NO_NUMBER)
        diskInfo.geometry.data.raw.sectorSize = cmdInfo.sectorSize;
    //if (cmdInfo.scheme
    /*
    //
    //
    //
    //
    //
    */
    printf("ALERT: CMDTYPE_EDIT_DISK hasn't been programmed yet\n");
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState applyCmd(ApplyCmdInfo* pInfo)
{
    switch (pInfo->pCmd->type)
    {
    case CMDTYPE_EDIT_DISK:
        return applyEditDisk(pInfo);
        break;
    default:
        fprintf(stderr, "unprogrammed command type given to %s (pCmd->type = %u)\n",
                        __func__, pInfo->pCmd->type);
        exit(EXIT_FAILURE);
        break;
    }
    return UTILS_ERRORSTATE_SUCCESS;
}
