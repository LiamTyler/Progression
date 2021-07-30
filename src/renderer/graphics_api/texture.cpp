#include "renderer/graphics_api/texture.hpp"
#include "core/assert.hpp"
#include "renderer/r_texture_manager.hpp"
#include <algorithm>
#include <cmath>

namespace PG
{
namespace Gfx
{
    

    void Texture::Free()
    {
        PG_ASSERT( m_image     != VK_NULL_HANDLE );
        PG_ASSERT( m_imageView != VK_NULL_HANDLE );
        PG_ASSERT( m_memory    != VK_NULL_HANDLE );

        vkDestroyImage( m_device, m_image, nullptr );
        vkDestroyImageView( m_device, m_imageView, nullptr );
        vkFreeMemory( m_device, m_memory, nullptr );
        if ( m_bindlessArrayIndex != PG_INVALID_TEXTURE_INDEX )
        {
            TextureManager::FreeSlot( m_bindlessArrayIndex );
            m_bindlessArrayIndex = PG_INVALID_TEXTURE_INDEX;
        }
        m_image       = VK_NULL_HANDLE;
        m_imageView   = VK_NULL_HANDLE;
        m_memory      = VK_NULL_HANDLE;
    }


    ImageType Texture::GetType() const { return m_desc.type; }
    PixelFormat Texture::GetPixelFormat() const { return m_desc.format; }
    uint32_t Texture::GetMipLevels() const { return m_desc.mipLevels; }
    uint32_t Texture::GetArrayLayers() const { return m_desc.arrayLayers; }
    uint32_t Texture::GetWidth() const { return m_desc.width; }
    uint32_t Texture::GetHeight() const { return m_desc.height; }
    uint32_t Texture::GetDepth() const { return m_desc.depth; }
    VkImage Texture::GetHandle() const { return m_image; }
    VkImageView Texture::GetView() const { return m_imageView; }
    VkDeviceMemory Texture::GetMemoryHandle() const { return m_memory; }
    uint16_t Texture::GetBindlessArrayIndex() const { return m_bindlessArrayIndex; }
    Sampler* Texture::GetSampler() const { return m_sampler; }
    Texture::operator bool() const { return m_image != VK_NULL_HANDLE; }

    void Texture::SetSampler( Sampler* sampler )
    {
        PG_ASSERT( sampler );
        m_sampler = sampler;
        PG_ASSERT( m_image == VK_NULL_HANDLE, "Changing sampler after image creation not supported yet" );
        if ( m_bindlessArrayIndex != PG_INVALID_TEXTURE_INDEX )
        {
            TextureManager::UpdateSampler( this );
        }
    }

} // namespace Gfx
} // namespace PG
