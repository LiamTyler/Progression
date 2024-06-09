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
    bool Create( u32 preferredWidth, u32 preferredHeight );
    bool Recreate( u32 preferredWidth, u32 preferredHeight );
    // returns false when a resize is needed
    bool AcquireNextImage( const Semaphore& presentCompleteSemaphore );
    void Free();
    operator bool() const;
    operator VkSwapchainKHR() const;

    u32 GetCurrentImageIndex() const;
    PixelFormat GetFormat() const;
    u32 GetWidth() const;
    u32 GetHeight() const;
    VkSwapchainKHR GetHandle() const;
    u32 GetNumImages() const;
    Texture& GetTexture();
    Texture& GetTextureAt( u32 index );

private:
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;
    PixelFormat m_imageFormat;
    u32 m_width;
    u32 m_height;
    u32 m_currentImageIdx;
    std::vector<Texture> m_textures;
};

} // namespace PG::Gfx
