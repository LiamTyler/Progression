#include "renderer/graphics_api/texture.hpp"
#include "core/assert.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/vulkan.hpp"
//#include "renderer/texture_manager.hpp"

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
        // if ( m_textureSlot != PG_INVALID_TEXTURE_INDEX )
        // {
        //     TextureManager::FreeSlot( m_textureSlot );
        //     m_textureSlot = PG_INVALID_TEXTURE_INDEX;
        // }
        m_image       = VK_NULL_HANDLE;
        m_imageView   = VK_NULL_HANDLE;
        m_memory      = VK_NULL_HANDLE;
    }


    unsigned char* Texture::GetPixelData() const
    {
        PG_ASSERT( false );
        return nullptr;
    }


    ImageType Texture::GetType() const
    {
        return m_desc.type;
    }


    PixelFormat Texture::GetPixelFormat() const
    {
        return m_desc.format;
    }


    uint32_t Texture::GetMipLevels() const
    {
        return m_desc.mipLevels;
    }


    uint32_t Texture::GetArrayLayers() const
    {
        return m_desc.arrayLayers;
    }


    uint32_t Texture::GetWidth() const
    {
        return m_desc.width;
    }


    uint32_t Texture::GetHeight() const
    {
        return m_desc.height;
    }


    uint32_t Texture::GetDepth() const
    {
        return m_desc.depth;
    }


    VkImage Texture::GetHandle() const
    {
        return m_image;
    }


    VkImageView Texture::GetView() const
    {
        return m_imageView;
    }


    VkDeviceMemory Texture::GetMemoryHandle() const
    {
        return m_memory;
    }


    uint16_t Texture::GetShaderSlot() const
    {
        return m_textureSlot;
    }


    Sampler* Texture::GetSampler() const
    {
        return m_sampler;
    }


    void Texture::SetSampler( Sampler* sampler )
    {
        PG_ASSERT( sampler );
        m_sampler = sampler;
        PG_ASSERT( false );
        //TextureManager::UpdateSampler( this );
    }


    Texture::operator bool() const
    {
        return m_image != VK_NULL_HANDLE;
    }


} // namespace Gfx
} // namespace PG
