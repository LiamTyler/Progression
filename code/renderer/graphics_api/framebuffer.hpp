#pragma once

#include "renderer/vulkan.hpp"

namespace PG
{
namespace Gfx
{

    class Framebuffer
    {
        friend class Device;
    public:
        Framebuffer() = default;

        void Free();
        operator bool() const;
        VkFramebuffer GetHandle() const;
        uint32_t      GetWidth() const;
        uint32_t      GetHeight() const;

    private:
        VkFramebuffer m_handle = VK_NULL_HANDLE;
        VkDevice      m_device = VK_NULL_HANDLE;
        uint32_t m_width;
        uint32_t m_height;
    };

} // namespace Gfx
} // namespace PG
