#include "renderer/graphics_api/pipeline.hpp"
#include "core/assert.hpp"
#include "renderer/r_globals.hpp"
#include "utils/bitops.hpp"

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
        ForEachBit( m_resourceLayout.descriptorSetMask, [&]( uint32_t set )
        {
            m_resourceLayout.sets[set].Free();
        });
        m_pipeline       = VK_NULL_HANDLE;
        m_pipelineLayout = VK_NULL_HANDLE;
    }


    VkPipeline Pipeline::GetHandle() const { return m_pipeline; }
    VkPipelineLayout Pipeline::GetLayoutHandle() const { return m_pipelineLayout; }
    const PipelineResourceLayout* Pipeline::GetResourceLayout() const { return &m_resourceLayout; }
    VkPipelineBindPoint Pipeline::GetPipelineBindPoint() const { return m_isCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS; };


    const DescriptorSetLayout* Pipeline::GetDescriptorSetLayout( int set ) const
    {
        PG_ASSERT( set < PG_MAX_NUM_DESCRIPTOR_SETS );
        PG_ASSERT( m_resourceLayout.descriptorSetMask & (1u << set) );

        return &m_resourceLayout.sets[set];
    }


    Pipeline::operator bool() const
    {
        return m_pipeline != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace PG
