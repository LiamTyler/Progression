#pragma once

#include <vulkan/vulkan.h>
#include "shared/assert.hpp"
#include "shared/platform_defines.hpp"

#if USING( SHIP_BUILD )
#define VK_CHECK_RESULT( f ) f
#else // #if USING( SHIP_BUILD )
#define VK_CHECK_RESULT( f ) 																\
{																						    \
    VkResult res = (f);																	    \
    if ( res != VK_SUCCESS )																\
    {																					    \
        printf( "Fatal : VkResult is %d in %s at line %d\n", res,  __FILE__, __LINE__ );    \
        PG_ASSERT( res == VK_SUCCESS );													    \
    }																					    \
}
#endif // #else // #if USING( SHIP_BUILD )

namespace PG
{
namespace Gfx
{

    uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

    void TransitionImageLayoutImmediate( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layers = 1 );

    bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

    VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1 );

} // namepspace Gfx
} // namepspace PG
