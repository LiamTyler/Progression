#pragma once

#include "renderer/graphics_api/descriptor.hpp"
#include "vulkan.hpp"

namespace PG::Gfx::TextureManager
{

enum class Usage : uint8_t
{
    NONE       = 0,
    READ       = 1 << 0,
    WRITE      = 1 << 1,
    READ_WRITE = READ | WRITE,
};
PG_DEFINE_ENUM_OPS( Usage );

void Init();
void Shutdown();
void Update();
uint16_t AddTexture( VkImageView imgView, Usage usage = Usage::READ_WRITE );
void RemoveTexture( uint16_t index );

const VkDescriptorSetLayout& GetBindlessSetLayout();
#if USING( PG_DESCRIPTOR_BUFFER )
const DescriptorBuffer& GetBindlessDescriptorBuffer();
#else  // #if USING( PG_DESCRIPTOR_BUFFER )
const VkDescriptorSet& GetBindlessSet();
#endif // #else // #if USING( PG_DESCRIPTOR_BUFFER )

} // namespace PG::Gfx::TextureManager
