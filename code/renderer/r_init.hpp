#pragma once

#include "shared/core_defines.hpp"

namespace PG::Gfx
{

bool R_Init( bool headless, u32 displayWidth = 0, u32 displayHeight = 0 );

void R_Shutdown();

} // namespace PG::Gfx
