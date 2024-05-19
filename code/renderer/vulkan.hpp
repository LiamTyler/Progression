#pragma once

#include "core/feature_defines.hpp"
#include "shared/assert.hpp"
#include "vma/vk_mem_alloc.h"
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#if USING( SHIP_BUILD )
#define VK_CHECK( f ) f
#else // #if USING( SHIP_BUILD )
#define VK_CHECK( f )                                                                                                                 \
    {                                                                                                                                 \
        VkResult pgVkRes = ( f );                                                                                                     \
        PG_ASSERT(                                                                                                                    \
            pgVkRes == VK_SUCCESS, "VkResult is %d (%s) in %s at line %d", pgVkRes, string_VkResult( pgVkRes ), __FILE__, __LINE__ ); \
    }
#endif // #else // #if USING( SHIP_BUILD )

namespace PG::Gfx
{

void LoadVulkanExtensions( VkDevice device );

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1 );

VkImageSubresourceRange ImageSubresourceRange( VkImageAspectFlags aspectFlags, uint32_t baseMipLevel = 0,
    uint32_t levelCount = VK_REMAINING_MIP_LEVELS, uint32_t baseArrayLayer = 0, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS );

VkSemaphoreSubmitInfo SemaphoreSubmitInfo( VkPipelineStageFlags2 stageMask, VkSemaphore semaphore );

} // namespace PG::Gfx
