#pragma once

#include "core/feature_defines.hpp"
#include "shared/assert.hpp"
#include "vma/vk_mem_alloc.h"
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#if USING( SHIP_BUILD )
    #define VK_CHECK_RESULT( f ) f
#else // #if USING( SHIP_BUILD )
    #define VK_CHECK_RESULT( f )                                                                                                 \
        {                                                                                                                        \
            VkResult vkRes = ( f );                                                                                              \
            if ( vkRes != VK_SUCCESS )                                                                                           \
            {                                                                                                                    \
                printf( "Fatal : VkResult is %d (%s) in %s at line %d\n", vkRes, string_VkResult( vkRes ), __FILE__, __LINE__ ); \
                PG_ASSERT( vkRes == VK_SUCCESS );                                                                                \
            }                                                                                                                    \
        }
#endif // #else // #if USING( SHIP_BUILD )

namespace PG::Gfx
{

void LoadVulkanExtensions( VkDevice device );

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

void TransitionImageLayoutImmediate(
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layers = 1 );

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1 );

VkImageSubresourceRange ImageSubresourceRange( VkImageAspectFlags aspectFlags, uint32_t baseMipLevel = 0,
    uint32_t levelCount = VK_REMAINING_MIP_LEVELS, uint32_t baseArrayLayer = 0, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS );

VkSemaphoreSubmitInfo SemaphoreSubmitInfo( VkPipelineStageFlags2 stageMask, VkSemaphore semaphore );

} // namespace PG::Gfx
