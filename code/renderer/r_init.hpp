#pragma once

#include <cstdint>

namespace PG::Gfx
{

bool R_Init( bool headless, uint32_t displayWidth = 0, uint32_t displayHeight = 0 );

void R_Shutdown();

} // namespace PG::Gfx
