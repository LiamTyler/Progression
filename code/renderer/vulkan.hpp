#pragma once

#include "core/feature_defines.hpp"
#include "shared/assert.hpp"
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include "vma/vk_mem_alloc.h"

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

static constexpr const char* REQUIRED_VK_EXTENSIONS[] =
{
#if USING( PG_RTX )
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
#endif // #if USING( PG_RTX )
#if USING( DEBUG_BUILD ) // perf hit
    VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
#endif // #if USING( DEBUG_BUILD )
    NULL
};

static constexpr const char* REQUIRED_VK_NON_HEADLESS_EXTENSIONS[] =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    NULL
};

void LoadVulkanExtensions( VkDevice device );

void* MakePNextChain( const std::vector<void*>& addresses );

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

void TransitionImageLayoutImmediate(
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layers = 1 );

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1 );

} // namespace PG::Gfx
