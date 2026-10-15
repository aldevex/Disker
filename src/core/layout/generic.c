#include "./generic.h"

#include "./disk/raw_img.h"

es_t lgeneric_create(const ConstSharedInfo* pInfo, CmdInfo* pCmdInfo, layout_DiskInfo* pDiskInfo)
{
    switch (pInfo->pCmd->info.createDisk.type)
    {
    case LAYOUT_DISKTYPE_RAW_IMAGE:
        return ldrawimg_create(pInfo, pCmdInfo, pDiskInfo);
    default:
        base_fatalErrorf("Unprogrammed disk type = %u given to %s.\n",
                        pInfo->pCmd->info.createDisk.type, __func__);
        base_halt(ES_FAILURE);
    }
}



es_t lgeneric_close(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo)
{
    switch (pInfo->pDiskInfo->diskLayout.type)
    {
    case LAYOUT_DISKTYPE_RAW_IMAGE:
        return ldrawimg_close(pInfo, pDiskInfo);
    default:
        base_fatalErrorf("Unprogrammed disk type = %u given to %s.\n",
                        pInfo->pDiskInfo->diskLayout.type, __func__);
        base_halt(ES_FAILURE);
    }
}

void lgeneric_free(const ConstSharedInfo* pInfo, layout_DiskInfo* pDiskInfo)
{
    // Close opened disk and free layout-specfic metadata memory
    switch (pInfo->pDiskInfo->diskLayout.type)
    {
    case LAYOUT_DISKTYPE_NULL:
    case LAYOUT_DISKTYPE_UNKNOWN:
        break;
    case LAYOUT_DISKTYPE_RAW_IMAGE:
        ldrawimg_free(pInfo, pDiskInfo);
        break;
    default:
        base_fatalErrorf("Unprogrammed disk type = %u at end of %s.\n",
                        pInfo->pDiskInfo->diskLayout.type, __func__);
        base_halt(ES_FAILURE);
    }
}
