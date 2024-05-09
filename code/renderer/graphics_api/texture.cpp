#include "renderer/graphics_api/texture.hpp"
#include "asset/types/gfx_image.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_texture_manager.hpp"
#include "shared/assert.hpp"
#include <algorithm>
#include <cmath>

namespace PG::Gfx
{

void Texture::Free()
{
    PG_ASSERT( m_image != VK_NULL_HANDLE );
    PG_ASSERT( m_imageView != VK_NULL_HANDLE );
    PG_ASSERT( m_allocation != nullptr );

    vkDestroyImageView( rg.device, m_imageView, nullptr );
    vmaDestroyImage( rg.device.GetAllocator(), m_image, m_allocation );
    if ( m_bindlessArrayIndex != PG_INVALID_TEXTURE_INDEX )
    {
        TextureManager::RemoveTexture( m_bindlessArrayIndex );
        m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;
    }
    m_image      = VK_NULL_HANDLE;
    m_imageView  = VK_NULL_HANDLE;
    m_allocation = nullptr;
}

ImageType Texture::GetType() const { return m_info.type; }
PixelFormat Texture::GetPixelFormat() const { return m_info.format; }
uint32_t Texture::GetMipLevels() const { return m_info.mipLevels; }
uint32_t Texture::GetArrayLayers() const { return m_info.arrayLayers; }
uint32_t Texture::GetWidth() const { return m_info.width; }
uint32_t Texture::GetHeight() const { return m_info.height; }
uint32_t Texture::GetDepth() const { return m_info.depth; }
VkExtent2D Texture::GetExtent2D() const { return { m_info.width, m_info.height }; }
VkExtent3D Texture::GetExtent3D() const { return { m_info.width, m_info.height, m_info.depth }; }
VkImage Texture::GetImage() const { return m_image; }
VkImageView Texture::GetView() const { return m_imageView; }
VmaAllocation Texture::GetAllocation() const { return m_allocation; }
uint16_t Texture::GetBindlessArrayIndex() const { return m_bindlessArrayIndex; }
Sampler* Texture::GetSampler() const { return m_sampler; }
Texture::operator bool() const { return m_image != VK_NULL_HANDLE; }
size_t Texture::GetTotalBytes() const { return m_info.TotalSizeInBytes(); }
size_t TextureCreateInfo::TotalSizeInBytes() const
{
    return CalculateTotalImageBytes( format, width, height, depth, arrayLayers, mipLevels );
}

bool ImageLayoutHasDepthAspect( ImageLayout layout )
{
    if ( layout == ImageLayout::DEPTH_STENCIL_ATTACHMENT || layout == ImageLayout::DEPTH_STENCIL_READ_ONLY ||
         layout == ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT || layout == ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY ||
         layout == ImageLayout::DEPTH_ATTACHMENT || layout == ImageLayout::DEPTH_READ_ONLY )
        return true;

    return false;
}

bool ImageLayoutHasStencilAspect( ImageLayout layout )
{
    if ( layout == ImageLayout::DEPTH_STENCIL_ATTACHMENT || layout == ImageLayout::DEPTH_STENCIL_READ_ONLY ||
         layout == ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT || layout == ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY ||
         layout == ImageLayout::STENCIL_ATTACHMENT || layout == ImageLayout::STENCIL_READ_ONLY )
        return true;

    return false;
}

std::string ImageLayoutToString( ImageLayout layout )
{
    switch ( layout )
    {
    case ImageLayout::UNDEFINED: return "UNDEFINED";
    case ImageLayout::GENERAL: return "GENERAL";
    case ImageLayout::COLOR_ATTACHMENT: return "COLOR_ATTACHMENT";
    case ImageLayout::DEPTH_STENCIL_ATTACHMENT: return "DEPTH_STENCIL_ATTACHMENT";
    case ImageLayout::DEPTH_STENCIL_READ_ONLY: return "DEPTH_STENCIL_READ_ONLY";
    case ImageLayout::SHADER_READ_ONLY: return "SHADER_READ_ONLY";
    case ImageLayout::TRANSFER_SRC: return "TRANSFER_SRC";
    case ImageLayout::TRANSFER_DST: return "TRANSFER_DST";
    case ImageLayout::PREINITIALIZED: return "PREINITIALIZED";
    case ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT: return "DEPTH_READ_ONLY_STENCIL_ATTACHMENT";
    case ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY: return "DEPTH_ATTACHMENT_STENCIL_READ_ONLY";
    case ImageLayout::DEPTH_ATTACHMENT: return "DEPTH_ATTACHMENT";
    case ImageLayout::DEPTH_READ_ONLY: return "DEPTH_READ_ONLY";
    case ImageLayout::STENCIL_ATTACHMENT: return "STENCIL_ATTACHMENT";
    case ImageLayout::STENCIL_READ_ONLY: return "STENCIL_READ_ONLY";
    case ImageLayout::READ_ONLY: return "READ_ONLY";
    case ImageLayout::ATTACHMENT: return "ATTACHMENT";
    case ImageLayout::PRESENT_SRC_KHR: return "PRESENT_SRC_KHR";
    default: PG_ASSERT( false, "Invalid image layout %d", Underlying( layout ) );
    }
}

} // namespace PG::Gfx
