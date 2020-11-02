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
        PG_ASSERT( m_numColorAttachments < 8, "Can't add more than 8 color attachments" );
        m_colorAttachmentDescriptors[m_numColorAttachments].format        = format;
        m_colorAttachmentDescriptors[m_numColorAttachments].loadAction    = loadAction;
        m_colorAttachmentDescriptors[m_numColorAttachments].storeAction   = storeAction;
        m_colorAttachmentDescriptors[m_numColorAttachments].clearColor    = clearColor;
        m_colorAttachmentDescriptors[m_numColorAttachments].initialLayout = initialLayout;
        m_colorAttachmentDescriptors[m_numColorAttachments].finalLayout   = finalLayout;
        ++m_numColorAttachments;
    }


    void RenderPassDescriptor::AddDepthAttachment( PixelFormat format, LoadAction loadAction, StoreAction storeAction, float clearValue, ImageLayout initialLayout, ImageLayout finalLayout )
    {
        PG_ASSERT( m_numDepthAttachments == 0, "Can't add more than 1 depth attachment" );
        m_depthAttachmentDescriptor.format        = format;
        m_depthAttachmentDescriptor.loadAction    = loadAction;
        m_depthAttachmentDescriptor.storeAction   = storeAction;
        m_depthAttachmentDescriptor.clearValue    = clearValue;
        m_depthAttachmentDescriptor.initialLayout = initialLayout;
        m_depthAttachmentDescriptor.finalLayout   = finalLayout;
        ++m_numDepthAttachments;
    }

    
    const ColorAttachmentDescriptor* RenderPassDescriptor::GetColorAttachment( uint32_t i ) const { return &m_colorAttachmentDescriptors[i]; }
    const DepthAttachmentDescriptor* RenderPassDescriptor::GetDepthAttachment() const { return &m_depthAttachmentDescriptor; }
    uint8_t RenderPassDescriptor::GetNumColorAttachments() const { return m_numColorAttachments; }
    uint8_t RenderPassDescriptor::GetNumDepthAttachments() const { return m_numDepthAttachments; }

    VkRenderPass RenderPass::GetHandle() const { return m_handle; }
    RenderPass::operator bool() const { return m_handle != VK_NULL_HANDLE; }


} // namespace Gfx
} // namespace PG
