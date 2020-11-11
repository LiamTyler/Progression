#pragma once

#include <string>

namespace PG
{

class Scene;

namespace RenderSystem
{

    bool Init( bool headless = false );

    void Shutdown();

    void Render( Scene* scene );

} // namespace RenderSystem
} // namespace PG
