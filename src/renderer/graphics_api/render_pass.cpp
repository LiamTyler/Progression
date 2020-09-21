#include "renderer/graphics_api/render_pass.hpp"
#include "core/assert.hpp"

namespace PG
{
namespace Gfx
{


    void RenderPass::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyRenderPass( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }
    

    VkRenderPass RenderPass::GetHandle() const
    {
        return m_handle;
    }
    

    RenderPass::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


} // namespace Gfx
} // namespace PG
