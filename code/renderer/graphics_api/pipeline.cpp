#include "renderer/graphics_api/pipeline.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/bitops.hpp"

namespace PG::Gfx
{

void Pipeline::Free()
{
    PG_ASSERT( m_pipeline != VK_NULL_HANDLE && m_pipelineLayout != VK_NULL_HANDLE );
    vkDestroyPipeline( rg.device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( rg.device, m_pipelineLayout, nullptr );
    m_pipeline       = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
}

VkPipeline Pipeline::GetHandle() const { return m_pipeline; }
VkPipelineLayout Pipeline::GetLayoutHandle() const { return m_pipelineLayout; }
VkPipelineBindPoint Pipeline::GetPipelineBindPoint() const { return m_bindPoint; };
VkShaderStageFlags Pipeline::GetPushConstantShaderStages() const { return m_stageFlags; }
PipelineType Pipeline::GetPipelineType() const { return m_pipelineType; }
uvec3 Pipeline::GetWorkgroupSize() const { return m_workgroupSize; }

Pipeline::operator bool() const { return m_pipeline != VK_NULL_HANDLE; }
Pipeline::operator VkPipeline() const { return m_pipeline; }

} // namespace PG::Gfx
