#pragma once

#include <vulkan/vulkan.h>

namespace PG
{
namespace Gfx
{

    uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

    void TransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layers = 1 );

    bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

    VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1 );

} // namepspace Gfx
} // namepspace PG
