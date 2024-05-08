#pragma once

#include "renderer/graphics_api/descriptor.hpp"
#include "vulkan.hpp"

namespace PG::Gfx::TextureManager
{

void Init();
void Shutdown();
void Update();
uint16_t AddTexture( VkImageView imgView );
void RemoveTexture( uint16_t index );

const VkDescriptorSetLayout& GetBindlessSetLayout();
#if USING( PG_DESCRIPTOR_BUFFER )
const DescriptorBuffer& GetBindlessDescriptorBuffer();
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
const VkDescriptorSet& GetBindlessSet();
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

} // namespace PG::Gfx::TextureManager
