#pragma once

#include "aoi.h"

#include <mutex>

enum class AOIType {
    ROOM,
    GRID,
    LINK
};

std::shared_ptr<AOI> create_aoi(AOIType aoi_type, size_t grid_size = 0);