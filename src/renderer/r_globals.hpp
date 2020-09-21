#pragma once

#include "renderer/graphics_api/device.hpp"

namespace PG
{
namespace Gfx
{

class Device;


struct R_Globals
{
    Gfx::Device* device;
};

extern R_Globals r_globals;

} // namespace Gfx
} // namespace PG