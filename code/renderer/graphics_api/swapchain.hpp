#pragma once

#include "renderer/graphics_api/synchronization.hpp"
#include "renderer/graphics_api/texture.hpp"
#include "renderer/vulkan.hpp"
#include <array>
#include <string>

#define GFX_MAX_SWAPCHAIN_IMAGES 3

namespace PG::Gfx
{

class Swapchain
{
public:
    bool Create( uint32_t preferredWidth, uint32_t preferredHeight );
    bool Recreate( uint32_t preferredWidth, uint32_t preferredHeight );
    // returns false when a resize is needed
    bool AcquireNextImage( const Semaphore& presentCompleteSemaphore );
    void Free();
    operator bool() const;
    operator VkSwapchainKHR() const;

    uint32_t GetCurrentImageIndex() const;
    PixelFormat GetFormat() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    VkSwapchainKHR GetHandle() const;
    uint32_t GetNumImages() const;
    Texture& GetTexture();
    Texture& GetTextureAt( uint32_t index );

private:
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;
    PixelFormat m_imageFormat;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_currentImageIdx;
    std::vector<Texture> m_textures;
};

} // namespace PG::Gfx
