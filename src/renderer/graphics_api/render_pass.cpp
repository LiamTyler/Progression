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


    std::string ImageLayoutToString( ImageLayout layout )
    {
        switch( layout )
        {
        case ImageLayout::UNDEFINED:
            return "UNDEFINED";
        case ImageLayout::GENERAL:
            return "GENERAL";
        case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
            return "COLOR_ATTACHMENT_OPTIMAL";
        case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return "DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
        case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return "DEPTH_STENCIL_READ_ONLY_OPTIMAL";
        case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
            return "SHADER_READ_ONLY_OPTIMAL";
        case ImageLayout::TRANSFER_SRC_OPTIMAL:
            return "TRANSFER_SRC_OPTIMAL";
        case ImageLayout::TRANSFER_DST_OPTIMAL:
            return "TRANSFER_DST_OPTIMAL";
        case ImageLayout::PREINITIALIZED:
            return "PREINITIALIZED";
        case ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            return "DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL";
        case ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            return "DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL";
        case ImageLayout::PRESENT_SRC_KHR:
            return "PRESENT_SRC_KHR";
        default:
            PG_ASSERT( false, "Invalid image layout '" + std::to_string( (int)layout ) + "'" );
        }
    }


} // namespace Gfx
} // namespace PG
