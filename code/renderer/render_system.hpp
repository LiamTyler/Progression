#pragma once
#pragma once

#include "graphics_api/pipeline.hpp"
#include <string>

namespace PG
{

class Scene;

namespace RenderSystem
{
bool Init( u32 sceneWidth, u32 sceneHeight, u32 displayWidth, u32 displayHeight, bool headless = false );
void Resize( u32 displayWidth, u32 displayHeight );
void Shutdown();
void Render();

void CreateTLAS( Scene* scene );

} // namespace RenderSystem
} // namespace PG
