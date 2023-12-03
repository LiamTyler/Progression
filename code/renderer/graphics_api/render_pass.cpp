#include "renderer/graphics_api/render_pass.hpp"
#include "shared/assert.hpp"

namespace PG::Gfx
{

void RenderPass::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkDestroyRenderPass( m_device, m_handle, nullptr );
    m_handle = VK_NULL_HANDLE;
}

void RenderPassDescriptor::AddColorAttachment( PixelFormat format, LoadAction loadAction, StoreAction storeAction,
    const vec4& clearColor, ImageLayout initialLayout, ImageLayout finalLayout )
{
    PG_ASSERT( numColorAttachments < MAX_COLOR_ATTACHMENTS, "Can't add more than %u color attachments", MAX_COLOR_ATTACHMENTS );
    colorAttachmentDescriptors[numColorAttachments].format        = format;
    colorAttachmentDescriptors[numColorAttachments].loadAction    = loadAction;
    colorAttachmentDescriptors[numColorAttachments].storeAction   = storeAction;
    colorAttachmentDescriptors[numColorAttachments].clearColor    = clearColor;
    colorAttachmentDescriptors[numColorAttachments].initialLayout = initialLayout;
    colorAttachmentDescriptors[numColorAttachments].finalLayout   = finalLayout;
    ++numColorAttachments;
}

void RenderPassDescriptor::AddDepthAttachment( PixelFormat format, LoadAction loadAction, StoreAction storeAction, float clearValue,
    ImageLayout initialLayout, ImageLayout finalLayout )
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

std::string LoadActionToString( LoadAction action )
{
    switch ( action )
    {
    case LoadAction::LOAD: return "LOAD";
    case LoadAction::CLEAR: return "CLEAR";
    case LoadAction::DONT_CARE: return "DONT_CARE";
    default: PG_ASSERT( false, "Invalid load action %d", Underlying( action ) );
    }
}

std::string StoreActionToString( StoreAction action )
{
    switch ( action )
    {
    case StoreAction::STORE: return "STORE";
    case StoreAction::DONT_CARE: return "DONT_CARE";
    default: PG_ASSERT( false, "Invalid store action %d", Underlying( action ) );
    }
}

std::string ImageLayoutToString( ImageLayout layout )
{
    switch ( layout )
    {
    case ImageLayout::UNDEFINED: return "UNDEFINED";
    case ImageLayout::GENERAL: return "GENERAL";
    case ImageLayout::COLOR_ATTACHMENT: return "COLOR_ATTACHMENT";
    case ImageLayout::DEPTH_STENCIL_ATTACHMENT: return "DEPTH_STENCIL_ATTACHMENT";
    case ImageLayout::DEPTH_STENCIL_READ_ONLY: return "DEPTH_STENCIL_READ_ONLY";
    case ImageLayout::SHADER_READ_ONLY: return "SHADER_READ_ONLY";
    case ImageLayout::TRANSFER_SRC: return "TRANSFER_SRC";
    case ImageLayout::TRANSFER_DST: return "TRANSFER_DST";
    case ImageLayout::PREINITIALIZED: return "PREINITIALIZED";
    case ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT: return "DEPTH_READ_ONLY_STENCIL_ATTACHMENT";
    case ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY: return "DEPTH_ATTACHMENT_STENCIL_READ_ONLY";
    case ImageLayout::DEPTH_ATTACHMENT: return "DEPTH_ATTACHMENT";
    case ImageLayout::DEPTH_READ_ONLY: return "DEPTH_READ_ONLY";
    case ImageLayout::STENCIL_ATTACHMENT: return "STENCIL_ATTACHMENT";
    case ImageLayout::STENCIL_READ_ONLY: return "STENCIL_READ_ONLY";
    case ImageLayout::READ_ONLY: return "READ_ONLY";
    case ImageLayout::ATTACHMENT: return "ATTACHMENT";
    case ImageLayout::PRESENT_SRC_KHR: return "PRESENT_SRC_KHR";
    default: PG_ASSERT( false, "Invalid image layout %d", Underlying( layout ) );
    }
}

} // namespace PG::Gfx
