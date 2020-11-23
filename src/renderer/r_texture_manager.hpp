#pragma once
#include <cstdint>
#include <vector>
#include "graphics_api/descriptor.hpp"

namespace PG
{
namespace Gfx
{

class Texture;

namespace TextureManager
{

    void Init();
    void Shutdown();
    uint16_t GetOpenSlot( Texture* texture );
    void FreeSlot( uint16_t slot );
    void UpdateSampler( Texture* texture );
    void UpdateDescriptors( const DescriptorSet& textureDescriptorSet );

} // namespace TextureManager
} // namespace Gfx
} // namespace PG
