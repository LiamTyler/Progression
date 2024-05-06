#pragma once

#include "vulkan.hpp"
#include <cstdint>

namespace PG::Gfx::TextureManager
{

void Init();
void Shutdown();
const VkDescriptorSetLayout& GetBindlessSetLayout();
const VkDescriptorSet& GetBindlessSet();
void Update();
uint16_t AddTexture( VkImageView imgView );
void RemoveTexture( uint16_t index );

} // namespace PG::Gfx::TextureManager
