#pragma once

#include <cstdint>

namespace PG::Gfx
{

bool R_Init( bool headless, uint32_t width = 0, uint32_t height = 0 );

void R_Shutdown();

} // namespace PG::Gfx
