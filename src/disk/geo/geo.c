#include "./geo.h"

geo_Type geo_fileTypeToGeoType(utils_FileType ft)
{
    switch (ft)
    {
    case UTILS_FILETYPE_RAW: return GEO_TYPE_RAW_IMAGE;
    case UTILS_FILETYPE_NOEXT: return GEO_TYPE_UNKNOWN;
    default: return GEO_TYPE_NULL;
    }
    return GEO_TYPE_NULL;
}
