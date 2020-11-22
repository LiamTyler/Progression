#include "r_renderpasses.hpp"
#include "graphics_api/pg_to_vulkan_types.hpp"
#include "r_globals.hpp"
#include "utils/logger.hpp"
#include <functional>

namespace PG
{
namespace Gfx
{

static RenderPass s_renderPasses[GFX_RENDER_PASS_COUNT];
static Framebuffer s_framebuffers[GFX_RENDER_PASS_COUNT];


static bool InitRP_DepthPrepass()
{
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.AddDepthAttachment( PixelFormat::DEPTH_32_FLOAT, LoadAction::CLEAR, StoreAction::STORE, 1.0f, ImageLayout::UNDEFINED, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
    s_renderPasses[GFX_RENDER_PASS_DEPTH_PREPASS] = r_globals.device.NewRenderPass( renderPassDesc, "DepthPreass" );

    VkImageView attachments[] = { r_globals.depthTex.GetView() };
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = s_renderPasses[GFX_RENDER_PASS_DEPTH_PREPASS].GetHandle();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = r_globals.swapchain.GetWidth();
    framebufferInfo.height          = r_globals.swapchain.GetHeight();
    framebufferInfo.layers          = 1;
    s_framebuffers[GFX_RENDER_PASS_DEPTH_PREPASS] = r_globals.device.NewFramebuffer( framebufferInfo, "DepthPrepass" );
    
    return s_renderPasses[GFX_RENDER_PASS_DEPTH_PREPASS] && s_framebuffers[GFX_RENDER_PASS_DEPTH_PREPASS];
}


static bool InitRP_Lit()
{
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.AddColorAttachment( PixelFormat::R16_G16_B16_A16_FLOAT, LoadAction::CLEAR, StoreAction::STORE, glm::vec4( 1 ), ImageLayout::UNDEFINED, ImageLayout::SHADER_READ_ONLY_OPTIMAL );
    renderPassDesc.AddDepthAttachment( PixelFormat::DEPTH_32_FLOAT, LoadAction::LOAD, StoreAction::STORE, 1.0f, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
    //renderPassDesc.AddDepthAttachment( PixelFormat::DEPTH_32_FLOAT, LoadAction::LOAD, StoreAction::STORE, 1.0f, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
    s_renderPasses[GFX_RENDER_PASS_LIT] = r_globals.device.NewRenderPass( renderPassDesc, "Lit" );
    
    VkImageView attachments[] = { r_globals.colorTex.GetView(), r_globals.depthTex.GetView() };
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = s_renderPasses[GFX_RENDER_PASS_LIT].GetHandle();
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = r_globals.swapchain.GetWidth();
    framebufferInfo.height          = r_globals.swapchain.GetHeight();
    framebufferInfo.layers          = 1;
    s_framebuffers[GFX_RENDER_PASS_LIT] = r_globals.device.NewFramebuffer( framebufferInfo, "Lit" );
    
    return s_renderPasses[GFX_RENDER_PASS_LIT] && s_framebuffers[GFX_RENDER_PASS_LIT];
}


static bool InitRP_Skybox()
{
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.AddColorAttachment( PixelFormat::R16_G16_B16_A16_FLOAT, LoadAction::LOAD, StoreAction::STORE, glm::vec4( 0 ), ImageLayout::COLOR_ATTACHMENT_OPTIMAL, ImageLayout::COLOR_ATTACHMENT_OPTIMAL );
    renderPassDesc.AddDepthAttachment( PixelFormat::DEPTH_32_FLOAT, LoadAction::LOAD, StoreAction::STORE, 1.0f, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
    s_renderPasses[GFX_RENDER_PASS_SKYBOX] = r_globals.device.NewRenderPass( renderPassDesc, "Skybox" );

    VkImageView attachments[] = { r_globals.colorTex.GetView(), r_globals.depthTex.GetView() };
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = s_renderPasses[GFX_RENDER_PASS_SKYBOX].GetHandle();
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = r_globals.swapchain.GetWidth();
    framebufferInfo.height          = r_globals.swapchain.GetHeight();
    framebufferInfo.layers          = 1;
    s_framebuffers[GFX_RENDER_PASS_SKYBOX] = r_globals.device.NewFramebuffer( framebufferInfo, "Skybox" );
    
    return s_renderPasses[GFX_RENDER_PASS_SKYBOX] && s_framebuffers[GFX_RENDER_PASS_SKYBOX];
}


static bool InitRP_PostProcess()
{
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.AddColorAttachment( VulkanToPGPixelFormat( r_globals.swapchain.GetFormat() ), LoadAction::DONT_CARE, StoreAction::STORE, glm::vec4( 0 ), ImageLayout::UNDEFINED, ImageLayout::PRESENT_SRC_KHR );
    s_renderPasses[GFX_RENDER_PASS_POST_PROCESS] = r_globals.device.NewRenderPass( renderPassDesc, "PostProcess" );

    VkImageView attachments[1];
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = s_renderPasses[GFX_RENDER_PASS_POST_PROCESS].GetHandle();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = r_globals.swapchain.GetWidth();
    framebufferInfo.height          = r_globals.swapchain.GetHeight();
    framebufferInfo.layers          = 1;

    for ( uint32_t i = 0; i < r_globals.swapchain.GetNumImages(); ++i )
    {
        attachments[0] = r_globals.swapchain.GetImageView( i );

        r_globals.swapchainFramebuffers[i] = r_globals.device.NewFramebuffer( framebufferInfo, "swapchain " + std::to_string( i ) );
        if ( !r_globals.swapchainFramebuffers[i] )
        {
            return false;
        }
    }

    return s_renderPasses[GFX_RENDER_PASS_POST_PROCESS];
}


static std::function< bool() > s_renderPassInitFunctions[GFX_RENDER_PASS_COUNT] =
{
    InitRP_DepthPrepass,
    InitRP_Lit,
    InitRP_Skybox,
    InitRP_PostProcess
};


bool InitRenderPasses()
{
    bool noErrors = true;
    for ( int i = 0; i < GFX_RENDER_PASS_COUNT; ++i )
    {
        if ( !s_renderPassInitFunctions[i]() )
        {
            LOG_ERR( "Failed to init render pass %d\n", i );
            return false;
        }
    }

    return noErrors;
}


void FreeRenderPasses()
{
    for ( int i = 0; i < GFX_RENDER_PASS_COUNT; ++i )
    {
        s_renderPasses[i].Free();
        if ( s_framebuffers[i] )
        {
            s_framebuffers[i].Free();
        }
    }
}


RenderPass* GetRenderPass( RenderPasses renderPassKey )
{
    return &s_renderPasses[renderPassKey];
}


Framebuffer* GetFramebuffer( RenderPasses renderPassKey )
{
    return &s_framebuffers[renderPassKey];
}


} // namespace Gfx
} // namespace PG