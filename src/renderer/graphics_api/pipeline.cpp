#include "renderer/graphics_api/pipeline.hpp"
#include "core/assert.hpp"
#include "renderer/r_globals.hpp"

namespace PG
{
namespace Gfx
{

    Viewport FullScreenViewport()
    {
        Viewport v;
        v.width  = static_cast< float >( r_globals.swapchain.GetWidth() );
        v.height = static_cast< float >( r_globals.swapchain.GetHeight() );
        return v;
    }


    Viewport CustomViewport( const float &w, const float&h )
    {
        Viewport v;
        v.width = w;
        v.height = h;
        return v;
    }


    Scissor FullScreenScissor()
    {
        Scissor s;
        s.width  = r_globals.swapchain.GetWidth();
        s.height = r_globals.swapchain.GetHeight();
        return s;
    }


    void Pipeline::Free()
    {
        PG_ASSERT( m_pipeline != VK_NULL_HANDLE && m_pipelineLayout != VK_NULL_HANDLE );
        vkDestroyPipeline( m_device, m_pipeline, nullptr );
        vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );
        m_pipeline       = VK_NULL_HANDLE;
        m_pipelineLayout = VK_NULL_HANDLE;
    }


    VkPipeline Pipeline::GetHandle() const
    {
        return m_pipeline;
    }


    VkPipelineLayout Pipeline::GetLayoutHandle() const
    {
        return m_pipelineLayout;
    }


    Pipeline::operator bool() const
    {
        return m_pipeline != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace PG
