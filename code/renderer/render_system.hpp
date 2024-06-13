#pragma once

#include "shared/core_defines.hpp"

namespace PG::RenderSystem
{

bool Init( u32 sceneWidth, u32 sceneHeight, u32 displayWidth, u32 displayHeight, bool headless = false );
void Resize( u32 displayWidth, u32 displayHeight );
void Shutdown();
void Render();

} // namespace PG::RenderSystem
