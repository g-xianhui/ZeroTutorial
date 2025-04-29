#include "aoi_factory.h"
#include "grid_aoi.h"

std::shared_ptr<AOI> create_aoi(AOIType aoi_type, size_t grid_size)
{
    switch (aoi_type)
    {
    case AOIType::ROOM:
        break;
    case AOIType::GRID:
    {
        std::shared_ptr<AOI> aoi = std::make_shared<GridAOI>(grid_size);
        return aoi;
    }
    case AOIType::LINK:
        break;
    default:
        break;
    }
    return nullptr;
}
