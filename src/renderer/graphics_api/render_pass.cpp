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


    void RenderPassDescriptor::AddColorAttachment( PixelFormat format, LoadAction loadAction, StoreAction storeAction, const glm::vec4& clearColor, ImageLayout initialLayout, ImageLayout finalLayout )
    {
        PG_ASSERT( numColorAttachments < 8, "Can't add more than 8 color attachments" );
        colorAttachmentDescriptors[numColorAttachments].format        = format;
        colorAttachmentDescriptors[numColorAttachments].loadAction    = loadAction;
        colorAttachmentDescriptors[numColorAttachments].storeAction   = storeAction;
        colorAttachmentDescriptors[numColorAttachments].clearColor    = clearColor;
        colorAttachmentDescriptors[numColorAttachments].initialLayout = initialLayout;
        colorAttachmentDescriptors[numColorAttachments].finalLayout   = finalLayout;
        ++numColorAttachments;
    }


    void RenderPassDescriptor::AddDepthAttachment( PixelFormat format, LoadAction loadAction, StoreAction storeAction, float clearValue, ImageLayout initialLayout, ImageLayout finalLayout )
    {
        PG_ASSERT( numDepthAttachments == 0, "Can't add more than 1 depth attachment" );
        depthAttachmentDescriptor.format        = format;
        depthAttachmentDescriptor.loadAction    = loadAction;
        depthAttachmentDescriptor.storeAction   = storeAction;
        depthAttachmentDescriptor.clearValue    = clearValue;
        depthAttachmentDescriptor.initialLayout = initialLayout;
        depthAttachmentDescriptor.finalLayout   = finalLayout;
        ++numDepthAttachments;
    }

    VkRenderPass RenderPass::GetHandle() const { return m_handle; }
    RenderPass::operator bool() const { return m_handle != VK_NULL_HANDLE; }


} // namespace Gfx
} // namespace PG
