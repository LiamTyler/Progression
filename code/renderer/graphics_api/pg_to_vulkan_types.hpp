#pragma once

#include "asset/types/shader.hpp"
#include "renderer/graphics_api.hpp"
#include "shared/assert.hpp"
#include <unordered_map>

namespace PG
{
namespace Gfx
{

    constexpr VkShaderStageFlagBits PGToVulkanShaderStage( ShaderStage stage )
    {
        VkShaderStageFlagBits convert[] =
        {
            VK_SHADER_STAGE_VERTEX_BIT,                  // VERTEX
            VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,    // TESSELLATION_CONTROL
            VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, // TESSELLATION_EVALUATION
            VK_SHADER_STAGE_GEOMETRY_BIT,                // GEOMETRY
            VK_SHADER_STAGE_FRAGMENT_BIT,                // FRAGMENT
            VK_SHADER_STAGE_COMPUTE_BIT,                 // COMPUTE
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( ShaderStage::NUM_SHADER_STAGES ) );

        return convert[static_cast< int >( stage )];
    }

    constexpr VkCompareOp PGToVulkanCompareFunction( CompareFunction op )
    {
        return static_cast< VkCompareOp >( op );
    }

    constexpr VkBlendFactor PGToVulkanBlendFactor( BlendFactor factor )
    {
        VkBlendFactor convert[] =
        {
            VK_BLEND_FACTOR_ZERO,                // ZERO
            VK_BLEND_FACTOR_ONE,                 // ONE
            VK_BLEND_FACTOR_SRC_COLOR,           // SRC_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, // ONE_MINUS_SRC_COLOR
            VK_BLEND_FACTOR_SRC_ALPHA,           // SRC_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // ONE_MINUS_SRC_ALPHA
            VK_BLEND_FACTOR_DST_COLOR,           // DST_COLOR
            VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, // ONE_MINUS_DST_COLOR
            VK_BLEND_FACTOR_DST_ALPHA,           // DST_ALPHA
            VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // ONE_MINUS_DST_ALPHA
            VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,  // SRC_ALPHA_SATURATE
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BlendFactor::NUM_BLEND_FACTORS ) );

        return convert[static_cast< int >( factor )];
    }

    constexpr VkBlendOp PGToVulkanBlendEquation( BlendEquation eq )
    {
        VkBlendOp convert[] =
        {
            VK_BLEND_OP_ADD,              // ADD
            VK_BLEND_OP_SUBTRACT,         // SUBTRACT
            VK_BLEND_OP_REVERSE_SUBTRACT, // REVERSE_SUBTRACT
            VK_BLEND_OP_MIN,              // MIN
            VK_BLEND_OP_MAX,              // MAX
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BlendEquation::NUM_BLEND_EQUATIONS) );

        return convert[static_cast< int >( eq )];
    }

    constexpr VkFrontFace PGToVulkanWindingOrder( WindingOrder order )
    {
        return static_cast< VkFrontFace >( order );
    }

    constexpr VkCullModeFlagBits PGToVulkanCullFace( CullFace face )
    {
        return static_cast< VkCullModeFlagBits >( face );
    }

    constexpr VkPolygonMode PGToVulkanPolygonMode( PolygonMode mode )
    {
        return static_cast< VkPolygonMode >( mode );
    }

    constexpr VkIndexType PGToVulkanIndexType( IndexType indexType )
    {
        return static_cast< VkIndexType >( indexType );
    }
    
    constexpr VkBufferUsageFlags PGToVulkanBufferType( BufferType type )
    {
        return static_cast< VkBufferUsageFlags >( type );
    }

    constexpr VkFormat PGToVulkanBufferDataType( BufferDataType type )
    {
        VkFormat convert[] =
        {
            VK_FORMAT_UNDEFINED,        // INVALID

            VK_FORMAT_R8_UINT,          // UCHAR
            VK_FORMAT_R8G8_UINT,        // UCHAR2
            VK_FORMAT_R8G8B8_UINT,      // UCHAR3
            VK_FORMAT_R8G8B8A8_UINT,    // UCHAR4

            VK_FORMAT_R8_SINT,          // CHAR
            VK_FORMAT_R8G8_SINT,        // CHAR2
            VK_FORMAT_R8G8B8_SINT,      // CHAR3
            VK_FORMAT_R8G8B8A8_SINT,    // CHAR4

            VK_FORMAT_R8_UNORM,         // UCHAR_NORM
            VK_FORMAT_R8G8_UNORM,       // UCHAR2_NORM
            VK_FORMAT_R8G8B8_UNORM,     // UCHAR3_NORM
            VK_FORMAT_R8G8B8A8_UNORM,   // UCHAR4_NORM

            VK_FORMAT_R8_SNORM,         // CHAR_NORM
            VK_FORMAT_R8G8_SNORM,       // CHAR2_NORM
            VK_FORMAT_R8G8B8_SNORM,     // CHAR3_NORM
            VK_FORMAT_R8G8B8A8_SNORM,   // CHAR4_NORM

            VK_FORMAT_R16_UINT,          // USHORT
            VK_FORMAT_R16G16_UINT,       // USHORT2
            VK_FORMAT_R16G16B16_UINT,    // USHORT3
            VK_FORMAT_R16G16B16A16_UINT, // USHORT4

            VK_FORMAT_R16_SINT,          // SHORT
            VK_FORMAT_R16G16_SINT,       // SHORT2
            VK_FORMAT_R16G16B16_SINT,    // SHORT3
            VK_FORMAT_R16G16B16A16_SINT, // SHORT4

            VK_FORMAT_R16_UNORM,          // USHORT_NORM
            VK_FORMAT_R16G16_UNORM,       // USHORT2_NORM
            VK_FORMAT_R16G16B16_UNORM,    // USHORT3_NORM
            VK_FORMAT_R16G16B16A16_UNORM, // USHORT4_NORM

            VK_FORMAT_R16_SNORM,          // SHORT_NORM
            VK_FORMAT_R16G16_SNORM,       // SHORT2_NORM
            VK_FORMAT_R16G16B16_SNORM,    // SHORT3_NORM
            VK_FORMAT_R16G16B16A16_SNORM, // SHORT4_NORM

            VK_FORMAT_R16_SFLOAT,           // HALF
            VK_FORMAT_R16G16_SFLOAT,        // HALF2
            VK_FORMAT_R16G16B16_SFLOAT,     // HALF3
            VK_FORMAT_R16G16B16A16_SFLOAT,  // HALF4

            VK_FORMAT_R32_SFLOAT,           // FLOAT
            VK_FORMAT_R32G32_SFLOAT,        // FLOAT2
            VK_FORMAT_R32G32B32_SFLOAT,     // FLOAT3
            VK_FORMAT_R32G32B32A32_SFLOAT,  // FLOAT4

            VK_FORMAT_R32_UINT,           // UINT
            VK_FORMAT_R32G32_UINT,        // UINT2
            VK_FORMAT_R32G32B32_UINT,     // UINT3
            VK_FORMAT_R32G32B32A32_UINT,  // UINT4

            VK_FORMAT_R32_UINT,           // INT
            VK_FORMAT_R32G32_UINT,        // INT2
            VK_FORMAT_R32G32B32_UINT,     // INT3
            VK_FORMAT_R32G32B32A32_UINT,  // INT4
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( BufferDataType::NUM_BUFFER_DATA_TYPE ) );

        return convert[static_cast< int >( type )];
    }

    constexpr VkMemoryPropertyFlags PGToVulkanMemoryType( MemoryType type )
    {
        return static_cast< VkMemoryPropertyFlags >( type );
    }

    constexpr VkVertexInputRate PGToVulkanVertexInputRate( VertexInputRate inputRate )
    {
        return static_cast< VkVertexInputRate >( inputRate );
    }

    constexpr VkPrimitiveTopology PGToVulkanPrimitiveType( PrimitiveType topology )
    {
        VkPrimitiveTopology convert[] =
        {
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST,     // POINTS
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST,      // LINES
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST ,     // LINE_STRIP
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // TRIANGLES
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, // TRIANGLE_STRIP
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,   // TRIANGLE_FAN
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PrimitiveType::NUM_PRIMITIVE_TYPE ) );

        return convert[static_cast< int >( topology )];
    }

    constexpr VkFilter PGToVulkanFilterMode( FilterMode mode )
    {
        return static_cast< VkFilter >( mode );
    }

    constexpr VkSamplerMipmapMode PGToVulkanMipFilter( MipFilterMode mode )
    {
        return static_cast< VkSamplerMipmapMode >( mode );
    }

    constexpr VkSamplerAddressMode PGToVulkanWrapMode( WrapMode mode )
    {
        return static_cast< VkSamplerAddressMode >( mode );
    }

    constexpr VkBorderColor PGToVulkanBorderColor( BorderColor color )
    {
        return static_cast< VkBorderColor >( color );
    }

    constexpr VkImageType PGToVulkanImageType( ImageType type )
    {
        VkImageType convert[] =
        {
            VK_IMAGE_TYPE_1D,        // TYPE_1D
            VK_IMAGE_TYPE_1D,        // TYPE_1D_ARRAY
            VK_IMAGE_TYPE_2D,        // TYPE_2D 
            VK_IMAGE_TYPE_2D,        // TYPE_2D_ARRAY
            VK_IMAGE_TYPE_2D,        // TYPE_2D_ARRAY
            VK_IMAGE_TYPE_2D,        // TYPE_CUBEMAP_ARRAY
            VK_IMAGE_TYPE_3D,        // TYPE_3D_ARRAY
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( ImageType::NUM_IMAGE_TYPES ) );

        return convert[static_cast< int >( type )];
    }

    constexpr VkFormat PGToVulkanPixelFormat( PixelFormat format )
    {
        VkFormat convert[] =
        {
            VK_FORMAT_UNDEFINED, // INVALID

            VK_FORMAT_R8_UNORM, // R8_UNORM
            VK_FORMAT_R8_SNORM, // R8_SNORM
            VK_FORMAT_R8_UINT,  // R8_UINT
            VK_FORMAT_R8_SINT,  // R8_SINT
            VK_FORMAT_R8_SRGB,  // R8_SRGB

            VK_FORMAT_R8G8_UNORM, // R8_G8_UNORM
            VK_FORMAT_R8G8_SNORM, // R8_G8_SNORM
            VK_FORMAT_R8G8_UINT,  // R8_G8_UINT
            VK_FORMAT_R8G8_SINT,  // R8_G8_SINT
            VK_FORMAT_R8G8_SRGB,  // R8_G8_SRGB

            VK_FORMAT_R8G8B8_UNORM, // R8_G8_B8_UNORM
            VK_FORMAT_R8G8B8_SNORM, // R8_G8_B8_SNORM
            VK_FORMAT_R8G8B8_UINT,  // R8_G8_B8_UINT
            VK_FORMAT_R8G8B8_SINT,  // R8_G8_B8_SINT
            VK_FORMAT_R8G8B8_SRGB,  // R8_G8_B8_SRGB

            VK_FORMAT_B8G8R8_UNORM, // B8_G8_R8_UNORM
            VK_FORMAT_B8G8R8_SNORM, // B8_G8_R8_SNORM
            VK_FORMAT_B8G8R8_UINT,  // B8_G8_R8_UINT
            VK_FORMAT_B8G8R8_SINT,  // B8_G8_R8_SINT
            VK_FORMAT_B8G8R8_SRGB,  // B8_G8_R8_SRGB

            VK_FORMAT_R8G8B8A8_UNORM, // R8_G8_B8_A8_UNORM
            VK_FORMAT_R8G8B8A8_SNORM, // R8_G8_B8_A8_SNORM
            VK_FORMAT_R8G8B8A8_UINT,  // R8_G8_B8_A8_UINT
            VK_FORMAT_R8G8B8A8_SINT,  // R8_G8_B8_A8_SINT
            VK_FORMAT_R8G8B8A8_SRGB,  // R8_G8_B8_A8_SRGB

            VK_FORMAT_B8G8R8A8_UNORM, // B8_G8_R8_A8_UNORM
            VK_FORMAT_B8G8R8A8_SNORM, // B8_G8_R8_A8_SNORM
            VK_FORMAT_B8G8R8A8_UINT,  // B8_G8_R8_A8_UINT
            VK_FORMAT_B8G8R8A8_SINT,  // B8_G8_R8_A8_SINT
            VK_FORMAT_B8G8R8A8_SRGB,  // B8_G8_R8_A8_SRGB

            VK_FORMAT_R16_UNORM,  // R16_UNORM
            VK_FORMAT_R16_SNORM,  // R16_SNORM
            VK_FORMAT_R16_UINT,   // R16_UINT
            VK_FORMAT_R16_SINT,   // R16_SINT
            VK_FORMAT_R16_SFLOAT, // R16_FLOAT

            VK_FORMAT_R16G16_UNORM,  // R16_G16_UNORM
            VK_FORMAT_R16G16_SNORM,  // R16_G16_SNORM
            VK_FORMAT_R16G16_UINT,   // R16_G16_UINT
            VK_FORMAT_R16G16_SINT,   // R16_G16_SINT
            VK_FORMAT_R16G16_SFLOAT, // R16_G16_FLOAT

            VK_FORMAT_R16G16B16_UNORM,  // R16_G16_B16_UNORM
            VK_FORMAT_R16G16B16_SNORM,  // R16_G16_B16_SNORM
            VK_FORMAT_R16G16B16_UINT,   // R16_G16_B16_UINT
            VK_FORMAT_R16G16B16_SINT,   // R16_G16_B16_SINT
            VK_FORMAT_R16G16B16_SFLOAT, // R16_G16_B16_FLOAT

            VK_FORMAT_R16G16B16A16_UNORM,  // R16_G16_B16_A16_UNORM
            VK_FORMAT_R16G16B16A16_SNORM,  // R16_G16_B16_A16_SNORM
            VK_FORMAT_R16G16B16A16_UINT,   // R16_G16_B16_A16_UINT
            VK_FORMAT_R16G16B16A16_SINT,   // R16_G16_B16_A16_SINT
            VK_FORMAT_R16G16B16A16_SFLOAT, // R16_G16_B16_A16_FLOAT

            VK_FORMAT_R32_UINT,   // R32_UINT
            VK_FORMAT_R32_SINT,   // R32_SINT
            VK_FORMAT_R32_SFLOAT, // R32_FLOAT

            VK_FORMAT_R32G32_UINT,   // R32_G32_UINT
            VK_FORMAT_R32G32_SINT,   // R32_G32_SINT
            VK_FORMAT_R32G32_SFLOAT, // R32_G32_FLOAT

            VK_FORMAT_R32G32B32_UINT,       // R32_G32_B32_UINT
            VK_FORMAT_R32G32B32_SINT,       // R32_G32_B32_SINT
            VK_FORMAT_R32G32B32_SFLOAT,     // R32_G32_B32_FLOAT

            VK_FORMAT_R32G32B32A32_UINT,    // R32_G32_B32_A32_UINT
            VK_FORMAT_R32G32B32A32_SINT,    // R32_G32_B32_A32_SINT
            VK_FORMAT_R32G32B32A32_SFLOAT,  // R32_G32_B32_A32_FLOAT

            VK_FORMAT_D16_UNORM,            // DEPTH_16_UNORM
            VK_FORMAT_D32_SFLOAT,           // DEPTH_32_FLOAT
            VK_FORMAT_D16_UNORM_S8_UINT,    // DEPTH_16_UNORM_STENCIL_8_UINT
            VK_FORMAT_D24_UNORM_S8_UINT,    // DEPTH_24_UNORM_STENCIL_8_UINT
            VK_FORMAT_D32_SFLOAT_S8_UINT,   // DEPTH_32_FLOAT_STENCIL_8_UINT
            VK_FORMAT_S8_UINT,              // STENCIL_8_UINT

            VK_FORMAT_BC1_RGB_UNORM_BLOCK,  // BC1_RGB_UNORM
            VK_FORMAT_BC1_RGB_SRGB_BLOCK,   // BC1_RGB_SRGB
            VK_FORMAT_BC1_RGBA_UNORM_BLOCK, // BC1_RGBA_UNORM
            VK_FORMAT_BC1_RGBA_SRGB_BLOCK,  // BC1_RGBA_SRGB
            VK_FORMAT_BC2_UNORM_BLOCK,      // BC2_UNORM
            VK_FORMAT_BC2_SRGB_BLOCK,       // BC2_SRGB
            VK_FORMAT_BC3_UNORM_BLOCK,      // BC3_UNORM
            VK_FORMAT_BC3_SRGB_BLOCK,       // BC3_SRGB
            VK_FORMAT_BC4_UNORM_BLOCK,      // BC4_UNORM
            VK_FORMAT_BC4_SNORM_BLOCK,      // BC4_SNORM
            VK_FORMAT_BC5_UNORM_BLOCK,      // BC5_UNORM
            VK_FORMAT_BC5_SNORM_BLOCK,      // BC5_SNORM
            VK_FORMAT_BC6H_UFLOAT_BLOCK,    // BC6H_UFLOAT
            VK_FORMAT_BC6H_SFLOAT_BLOCK,    // BC6H_SFLOAT
            VK_FORMAT_BC7_UNORM_BLOCK,      // BC7_UNORM
            VK_FORMAT_BC7_SRGB_BLOCK,       // BC7_SRGB
        };

        static_assert( ARRAY_COUNT( convert ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return convert[static_cast< int >( format )];
    }

    inline PixelFormat VulkanToPGPixelFormat( VkFormat format )
    {
        std::unordered_map< VkFormat, PixelFormat > convert =
        {
            { VK_FORMAT_UNDEFINED,              PixelFormat::INVALID },
            { VK_FORMAT_R8_UNORM,               PixelFormat::R8_UNORM },
            { VK_FORMAT_R8_SNORM,               PixelFormat::R8_SNORM },
            { VK_FORMAT_R8_UINT,                PixelFormat::R8_UINT },
            { VK_FORMAT_R8_SINT,                PixelFormat::R8_SINT },
            { VK_FORMAT_R8_SRGB,                PixelFormat::R8_SRGB },
            { VK_FORMAT_R8G8_UNORM,             PixelFormat::R8_G8_UNORM },
            { VK_FORMAT_R8G8_SNORM,             PixelFormat::R8_G8_SNORM },
            { VK_FORMAT_R8G8_UINT,              PixelFormat::R8_G8_UINT },
            { VK_FORMAT_R8G8_SINT,              PixelFormat::R8_G8_SINT },
            { VK_FORMAT_R8G8_SRGB,              PixelFormat::R8_G8_SRGB },
            { VK_FORMAT_R8G8B8_UNORM,           PixelFormat::R8_G8_B8_UNORM },
            { VK_FORMAT_R8G8B8_SNORM,           PixelFormat::R8_G8_B8_SNORM },
            { VK_FORMAT_R8G8B8_UINT,            PixelFormat::R8_G8_B8_UINT },
            { VK_FORMAT_R8G8B8_SINT,            PixelFormat::R8_G8_B8_SINT },
            { VK_FORMAT_R8G8B8_SRGB,            PixelFormat::R8_G8_B8_SRGB },
            { VK_FORMAT_B8G8R8_UNORM,           PixelFormat::B8_G8_R8_UNORM },
            { VK_FORMAT_B8G8R8_SNORM,           PixelFormat::B8_G8_R8_SNORM },
            { VK_FORMAT_B8G8R8_UINT,            PixelFormat::B8_G8_R8_UINT },
            { VK_FORMAT_B8G8R8_SINT,            PixelFormat::B8_G8_R8_SINT },
            { VK_FORMAT_B8G8R8_SRGB,            PixelFormat::B8_G8_R8_SRGB },
            { VK_FORMAT_R8G8B8A8_UNORM,         PixelFormat::R8_G8_B8_A8_UNORM },
            { VK_FORMAT_R8G8B8A8_SNORM,         PixelFormat::R8_G8_B8_A8_SNORM },
            { VK_FORMAT_R8G8B8A8_UINT,          PixelFormat::R8_G8_B8_A8_UINT },
            { VK_FORMAT_R8G8B8A8_SINT,          PixelFormat::R8_G8_B8_A8_SINT },
            { VK_FORMAT_R8G8B8A8_SRGB,          PixelFormat::R8_G8_B8_A8_SRGB },
            { VK_FORMAT_B8G8R8A8_UNORM,         PixelFormat::B8_G8_R8_A8_UNORM },
            { VK_FORMAT_B8G8R8A8_SNORM,         PixelFormat::B8_G8_R8_A8_SNORM },
            { VK_FORMAT_B8G8R8A8_UINT,          PixelFormat::B8_G8_R8_A8_UINT },
            { VK_FORMAT_B8G8R8A8_SINT,          PixelFormat::B8_G8_R8_A8_SINT },
            { VK_FORMAT_B8G8R8A8_SRGB,          PixelFormat::B8_G8_R8_A8_SRGB },
            { VK_FORMAT_R16_UNORM,              PixelFormat::R16_UNORM },
            { VK_FORMAT_R16_SNORM,              PixelFormat::R16_SNORM },
            { VK_FORMAT_R16_UINT,               PixelFormat::R16_UINT },
            { VK_FORMAT_R16_SINT,               PixelFormat::R16_SINT },
            { VK_FORMAT_R16_SFLOAT,             PixelFormat::R16_FLOAT },
            { VK_FORMAT_R16G16_UNORM,           PixelFormat::R16_G16_UNORM },
            { VK_FORMAT_R16G16_SNORM,           PixelFormat::R16_G16_SNORM },
            { VK_FORMAT_R16G16_UINT,            PixelFormat::R16_G16_UINT },
            { VK_FORMAT_R16G16_SINT,            PixelFormat::R16_G16_SINT },
            { VK_FORMAT_R16G16_SFLOAT,          PixelFormat::R16_G16_FLOAT },
            { VK_FORMAT_R16G16B16_UNORM,        PixelFormat::R16_G16_B16_UNORM },
            { VK_FORMAT_R16G16B16_SNORM,        PixelFormat::R16_G16_B16_SNORM },
            { VK_FORMAT_R16G16B16_UINT,         PixelFormat::R16_G16_B16_UINT },
            { VK_FORMAT_R16G16B16_SINT,         PixelFormat::R16_G16_B16_SINT },
            { VK_FORMAT_R16G16B16_SFLOAT,       PixelFormat::R16_G16_B16_FLOAT },
            { VK_FORMAT_R16G16B16A16_UNORM,     PixelFormat::R16_G16_B16_A16_UNORM },
            { VK_FORMAT_R16G16B16A16_SNORM,     PixelFormat::R16_G16_B16_A16_SNORM },
            { VK_FORMAT_R16G16B16A16_UINT,      PixelFormat::R16_G16_B16_A16_UINT },
            { VK_FORMAT_R16G16B16A16_SINT,      PixelFormat::R16_G16_B16_A16_SINT },
            { VK_FORMAT_R16G16B16A16_SFLOAT,    PixelFormat::R16_G16_B16_A16_FLOAT },
            { VK_FORMAT_R32_UINT,               PixelFormat::R32_UINT },
            { VK_FORMAT_R32_SINT,               PixelFormat::R32_SINT },
            { VK_FORMAT_R32_SFLOAT,             PixelFormat::R32_FLOAT },
            { VK_FORMAT_R32G32_UINT,            PixelFormat::R32_G32_UINT },
            { VK_FORMAT_R32G32_SINT,            PixelFormat::R32_G32_SINT },
            { VK_FORMAT_R32G32_SFLOAT,          PixelFormat::R32_G32_FLOAT },
            { VK_FORMAT_R32G32B32_UINT,         PixelFormat::R32_G32_B32_UINT },
            { VK_FORMAT_R32G32B32_SINT,         PixelFormat::R32_G32_B32_SINT },
            { VK_FORMAT_R32G32B32_SFLOAT,       PixelFormat::R32_G32_B32_FLOAT },
            { VK_FORMAT_R32G32B32A32_UINT,      PixelFormat::R32_G32_B32_A32_UINT },
            { VK_FORMAT_R32G32B32A32_SINT,      PixelFormat::R32_G32_B32_A32_SINT },
            { VK_FORMAT_R32G32B32A32_SFLOAT,    PixelFormat::R32_G32_B32_A32_FLOAT },
            { VK_FORMAT_D16_UNORM,              PixelFormat::DEPTH_16_UNORM },
            { VK_FORMAT_D32_SFLOAT,             PixelFormat::DEPTH_32_FLOAT },
            { VK_FORMAT_D16_UNORM_S8_UINT,      PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT },
            { VK_FORMAT_D24_UNORM_S8_UINT,      PixelFormat::DEPTH_24_UNORM_STENCIL_8_UINT },
            { VK_FORMAT_D32_SFLOAT_S8_UINT,     PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT },
            { VK_FORMAT_S8_UINT,                PixelFormat::STENCIL_8_UINT },
            { VK_FORMAT_BC1_RGB_UNORM_BLOCK,    PixelFormat::BC1_RGB_UNORM },
            { VK_FORMAT_BC1_RGB_SRGB_BLOCK,     PixelFormat::BC1_RGB_SRGB },
            { VK_FORMAT_BC1_RGBA_UNORM_BLOCK,   PixelFormat::BC1_RGBA_UNORM },
            { VK_FORMAT_BC1_RGBA_SRGB_BLOCK,    PixelFormat::BC1_RGBA_SRGB },
            { VK_FORMAT_BC2_UNORM_BLOCK,        PixelFormat::BC2_UNORM },
            { VK_FORMAT_BC2_SRGB_BLOCK,         PixelFormat::BC2_SRGB },
            { VK_FORMAT_BC3_UNORM_BLOCK,        PixelFormat::BC3_UNORM },
            { VK_FORMAT_BC3_SRGB_BLOCK,         PixelFormat::BC3_SRGB },
            { VK_FORMAT_BC4_UNORM_BLOCK,        PixelFormat::BC4_UNORM },
            { VK_FORMAT_BC4_SNORM_BLOCK,        PixelFormat::BC4_SNORM },
            { VK_FORMAT_BC5_UNORM_BLOCK,        PixelFormat::BC5_UNORM },
            { VK_FORMAT_BC5_SNORM_BLOCK,        PixelFormat::BC5_SNORM },
            { VK_FORMAT_BC6H_UFLOAT_BLOCK,      PixelFormat::BC6H_UFLOAT },
            { VK_FORMAT_BC6H_SFLOAT_BLOCK,      PixelFormat::BC6H_SFLOAT },
            { VK_FORMAT_BC7_UNORM_BLOCK,        PixelFormat::BC7_UNORM },
            { VK_FORMAT_BC7_SRGB_BLOCK,         PixelFormat::BC7_SRGB },
        };

        PG_ASSERT( convert.find( format ) != convert.end() );

        return convert[format];
    }

    constexpr VkCommandBufferUsageFlags PGToVulkanCommandBufferUsage( CommandBufferUsage flags )
    {
        return static_cast< VkCommandBufferUsageFlags >( flags );
    }

    constexpr VkCommandPoolCreateFlags PGToVulkanCommandPoolCreateFlags( CommandPoolCreateFlags flags )
    {
        return static_cast< VkCommandPoolCreateFlags >( flags );
    }

    constexpr VkAttachmentLoadOp PGToVulkanLoadAction( LoadAction op )
    {
        return static_cast< VkAttachmentLoadOp >( op );
    }

    constexpr VkAttachmentStoreOp PGToVulkanStoreAction( StoreAction op )
    {
        return static_cast< VkAttachmentStoreOp >( op );
    }

    constexpr VkImageLayout PGToVulkanImageLayout( ImageLayout op )
    {
        return static_cast< VkImageLayout >( op );
    }

    constexpr VkAccessFlags PGToVulkanAccessFlags( AccessFlags flags )
    {
        return static_cast< VkAccessFlags >( flags );
    }

    constexpr VkPipelineStageFlags PGToVulkanPipelineStageFlags( PipelineStageFlags flags )
    {
        return static_cast< VkPipelineStageFlags >( flags );
    }

} // namespace Gfx
} // namespace PG
