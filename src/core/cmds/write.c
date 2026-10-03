#include "./cmds.h"

utils_ErrorState writeEditDisk(WriteCmdInfo* pWrtieInfo)
{
    CmdInfoEditDisk* pCmdInfo = &pWrtieInfo->pCmd->info.editDisk;
    DiskInfo* pDiskInfo = pWrtieInfo->pDiskInfo;
    // Extend file size (shrinking results in error in state validation)
    if (pCmdInfo->size != UTILS_SIZESIG_NO_NUMBER
    && pCmdInfo->size != pDiskInfo->og.format.data.raw.size)
    {
        return utils_extendFile(&vwstr(&pDiskInfo->path), pDiskInfo->handle,
                            pCmdInfo->size -pDiskInfo->og.format.data.raw.size);
    }
    return UTILS_ERRORSTATE_SUCCESS;
}

utils_ErrorState writeCmd(WriteCmdInfo* pInfo)
{
    switch (pInfo->pCmd->type)
    {
    case CMDTYPE_EDIT_DISK:
        return writeEditDisk(pInfo);
        break;
    default:
        fprintf(stderr, "unprogrammed command type given to %s (pCmd->type = %u)\n",
                        __func__, pInfo->pCmd->type);
        exit(EXIT_FAILURE);
        break;
    }
    return UTILS_ERRORSTATE_SUCCESS;
}
