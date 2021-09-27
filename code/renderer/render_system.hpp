#pragma once

#include <string>

namespace PG
{

class Scene;

namespace RenderSystem
{

    bool Init( uint32_t sceneWidth, uint32_t sceneHeight, bool headless = false );

    void Shutdown();

    void Render( Scene* scene );

} // namespace RenderSystem
} // namespace PG
