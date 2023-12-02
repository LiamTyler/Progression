#pragma once

#include "graphics_api/descriptor.hpp"
#include <cstdint>
#include <vector>

namespace PG::Gfx
{

class Texture;

namespace TextureManager
{

void Init();
void Shutdown();
uint16_t GetOpenSlot( Texture* texture );
void FreeSlot( uint16_t slot );
void UpdateDescriptors( const DescriptorSet& textureDescriptorSet );

} // namespace TextureManager
} // namespace PG::Gfx
