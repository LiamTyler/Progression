#include "renderer/graphics_api/texture.hpp"
#include "core/assert.hpp"
//#include "renderer/texture_manager.hpp"
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
        // if ( m_textureSlot != PG_INVALID_TEXTURE_INDEX )
        // {
        //     TextureManager::FreeSlot( m_textureSlot );
        //     m_textureSlot = PG_INVALID_TEXTURE_INDEX;
        // }
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
    uint16_t Texture::GetShaderSlot() const { return m_textureSlot; }
    Sampler* Texture::GetSampler() const { return m_sampler; }
    Texture::operator bool() const { return m_image != VK_NULL_HANDLE; }

    void Texture::SetSampler( Sampler* sampler )
    {
        PG_ASSERT( sampler );
        m_sampler = sampler;
        PG_ASSERT( m_image == VK_NULL_HANDLE, "Changing sampler after image creation not supported yet" );
        //TextureManager::UpdateSampler( this );
    }

} // namespace Gfx


    uint32_t CalculateNumMips( uint32_t width, uint32_t height )
    {
        uint32_t largestDim = std::max( width, height );
        if ( largestDim == 0 )
        {
            return 0;
        }

        return 1 + static_cast< uint32_t >( std::log2f( static_cast< float >( largestDim ) ) );
    }


    size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips )
    {
        PG_ASSERT( width > 0 && height > 0 );
        PG_ASSERT( !PixelFormatIsCompressed( format ), "compressed format not supported yet" );
        uint32_t w = width;
        uint32_t h = height;
        if ( numMips == 0 )
        {
            numMips = CalculateNumMips( w, h );
        }
        int bytesPerPixel = NumBytesPerPixel( format );
        size_t currentSize = 0;
        for ( uint32_t mipLevel = 0; mipLevel < numMips; ++mipLevel )
        {
            currentSize += w * h * bytesPerPixel;
            w = std::max( 1u, w >> 1 );
            h = std::max( 1u, h >> 1 );
        }

        return currentSize;
    }


    size_t CalculateTotalImageBytes( PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        size_t totalBytes = depth * arrayLayers * CalculateTotalFaceSizeWithMips( width, height, format, mipLevels );

        return totalBytes;
    }

} // namespace PG
