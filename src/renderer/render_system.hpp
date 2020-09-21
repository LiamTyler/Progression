#pragma once

#include <string>

namespace PG
{

class Scene;

namespace Gfx
{
    class SamplerDescriptor;
    class Sampler;
} // namespace Gfx

namespace RenderSystem
{

    bool Init();

    void Shutdown();

    void Render( Scene* scene );

} // namespace RenderSystem
} // namespace PG
