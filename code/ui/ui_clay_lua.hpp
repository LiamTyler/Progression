#pragma once

#include "asset/types/font.hpp"

namespace PG::UI
{

bool Init_ClayLua();
void Shutdown_ClayLua();

Font* GetFontById( u16 fontID );

} // namespace PG::UI
