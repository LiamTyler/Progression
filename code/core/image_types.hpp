#pragma once

#include "shared/core_defines.hpp"

// This is silly to have a file just for this, but im not sure where
// else to put it currently. Both the rendering code + non-rendering code
// need it, and I don't want to drag a lot of dependencies to either side

namespace PG
{

enum class ImageType : u8
{
    TYPE_1D            = 0,
    TYPE_1D_ARRAY      = 1,
    TYPE_2D            = 2,
    TYPE_2D_ARRAY      = 3,
    TYPE_CUBEMAP       = 4,
    TYPE_CUBEMAP_ARRAY = 5,
    TYPE_3D            = 6,

    NUM_IMAGE_TYPES
};

} // namespace PG
