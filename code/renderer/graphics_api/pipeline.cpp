#include "renderer/graphics_api/pipeline.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/bitops.hpp"

namespace PG::Gfx
{

// copied from Vulkan-Samples, with some added cases
PipelineStageFlags GetPipelineStageFlags( ImageLayout imageLayout )
{
    switch ( imageLayout )
    {
    case ImageLayout::UNDEFINED: return PipelineStageFlags::TOP_OF_PIPE_BIT;
    case ImageLayout::PREINITIALIZED: return PipelineStageFlags::HOST_BIT;
    case ImageLayout::TRANSFER_DST:
    case ImageLayout::TRANSFER_SRC: return PipelineStageFlags::TRANSFER_BIT;
    case ImageLayout::COLOR_ATTACHMENT: return PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT_BIT;
    case ImageLayout::DEPTH_STENCIL_ATTACHMENT:
    case ImageLayout::DEPTH_ATTACHMENT:
    case ImageLayout::STENCIL_ATTACHMENT: return PipelineStageFlags::EARLY_FRAGMENT_TESTS_BIT | PipelineStageFlags::LATE_FRAGMENT_TESTS_BIT;
    case ImageLayout::SHADER_READ_ONLY:
    case ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY:
    case ImageLayout::DEPTH_READ_ONLY:
    case ImageLayout::STENCIL_READ_ONLY: return PipelineStageFlags::VERTEX_SHADER_BIT | PipelineStageFlags::FRAGMENT_SHADER_BIT;
    case ImageLayout::PRESENT_SRC_KHR: return PipelineStageFlags::BOTTOM_OF_PIPE_BIT;
    case ImageLayout::GENERAL:
        PG_ASSERT( false, "Don't know how to get a meaningful VkPipelineStageFlags for VK_IMAGE_LAYOUT_GENERAL! Don't use it!" );
        return PipelineStageFlags::NONE;
    default: PG_ASSERT( false, "unknown ImageLayout in GetPipelineStageFlags" ); return PipelineStageFlags::NONE;
    }
}

void Pipeline::Free()
{
    PG_ASSERT( m_pipeline != VK_NULL_HANDLE && m_pipelineLayout != VK_NULL_HANDLE );
    vkDestroyPipeline( m_device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );
    ForEachBit( m_resourceLayout.descriptorSetMask, [&]( uint32_t set ) { m_resourceLayout.sets[set].Free(); } );
    m_pipeline       = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
}

VkPipeline Pipeline::GetHandle() const { return m_pipeline; }
VkPipelineLayout Pipeline::GetLayoutHandle() const { return m_pipelineLayout; }
const PipelineResourceLayout* Pipeline::GetResourceLayout() const { return &m_resourceLayout; }
VkPipelineBindPoint Pipeline::GetPipelineBindPoint() const
{
    return m_isCompute ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
};
VkShaderStageFlags Pipeline::GetPushConstantShaderStages() const { return m_resourceLayout.pushConstantRange.stageFlags; }

const DescriptorSetLayout* Pipeline::GetDescriptorSetLayout( int set ) const
{
    PG_ASSERT( set < PG_MAX_NUM_DESCRIPTOR_SETS );
    PG_ASSERT( m_resourceLayout.descriptorSetMask & ( 1u << set ) );

    return &m_resourceLayout.sets[set];
}

Pipeline::operator bool() const { return m_pipeline != VK_NULL_HANDLE; }

} // namespace PG::Gfx
