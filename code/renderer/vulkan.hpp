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

u32 FindMemoryType( u32 typeFilter, VkMemoryPropertyFlags properties );

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels = 1, u32 layers = 1 );

VkImageSubresourceRange ImageSubresourceRange( VkImageAspectFlags aspectFlags, u32 baseMipLevel = 0,
    u32 levelCount = VK_REMAINING_MIP_LEVELS, u32 baseArrayLayer = 0, u32 layerCount = VK_REMAINING_ARRAY_LAYERS );

VkSemaphoreSubmitInfo SemaphoreSubmitInfo( VkPipelineStageFlags2 stageMask, VkSemaphore semaphore );

} // namespace PG::Gfx
