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

    vkDestroyImageView( m_device, m_imageView, nullptr );
    vmaDestroyImage( rg.device.GetAllocator(), m_image, m_allocation );
    if ( m_bindlessArrayIndex != PG_INVALID_TEXTURE_INDEX )
    {
        //TextureManager::FreeSlot( m_bindlessArrayIndex );
        m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;
    }
    m_image      = VK_NULL_HANDLE;
    m_imageView  = VK_NULL_HANDLE;
    m_allocation = nullptr;
}

ImageType Texture::GetType() const { return m_desc.type; }
PixelFormat Texture::GetPixelFormat() const { return m_desc.format; }
uint32_t Texture::GetMipLevels() const { return m_desc.mipLevels; }
uint32_t Texture::GetArrayLayers() const { return m_desc.arrayLayers; }
uint32_t Texture::GetWidth() const { return m_desc.width; }
uint32_t Texture::GetHeight() const { return m_desc.height; }
uint32_t Texture::GetDepth() const { return m_desc.depth; }
VkExtent2D Texture::GetExtent2D() const { return { m_desc.width, m_desc.height }; }
VkExtent3D Texture::GetExtent3D() const { return { m_desc.width, m_desc.height, m_desc.depth }; }
VkImage Texture::GetImage() const { return m_image; }
VkImageView Texture::GetView() const { return m_imageView; }
VmaAllocation Texture::GetAllocation() const { return m_allocation; }
uint16_t Texture::GetBindlessArrayIndex() const { return m_bindlessArrayIndex; }
Sampler* Texture::GetSampler() const { return m_sampler; }
Texture::operator bool() const { return m_image != VK_NULL_HANDLE; }
size_t Texture::GetTotalBytes() const { return m_desc.TotalSizeInBytes(); }
size_t TextureCreateInfo::TotalSizeInBytes() const
{
    return CalculateTotalImageBytes( format, width, height, depth, arrayLayers, mipLevels );
}

} // namespace PG::Gfx
