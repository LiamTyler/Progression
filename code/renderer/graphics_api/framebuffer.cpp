#include "renderer/graphics_api/framebuffer.hpp"
#include "shared/assert.hpp"

namespace PG::Gfx
{

void Framebuffer::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkDestroyFramebuffer( m_device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}

Framebuffer::operator bool() const { return m_handle != VK_NULL_HANDLE; }

VkFramebuffer Framebuffer::GetHandle() const { return m_handle; }
uint32_t Framebuffer::GetWidth() const { return m_width; }
uint32_t Framebuffer::GetHeight() const { return m_height; }

} // namespace PG::Gfx
