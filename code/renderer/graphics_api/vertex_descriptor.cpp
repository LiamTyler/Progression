#include "renderer/graphics_api/vertex_descriptor.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"

namespace PG
{
namespace Gfx
{

VertexBindingDescriptor::VertexBindingDescriptor( uint32_t _binding, uint32_t _stride, VertexInputRate _inputRate )
    : binding( _binding ), stride( _stride ), inputRate( _inputRate )
{
}

VertexAttributeDescriptor::VertexAttributeDescriptor( uint32_t loc, uint32_t bind, BufferFormat _format, uint32_t _offset )
    : location( loc ), binding( bind ), format( _format ), offset( _offset )
{
}

VertexInputDescriptor VertexInputDescriptor::Create(
    uint8_t numBinding, VertexBindingDescriptor* bindingDesc, uint8_t numAttrib, VertexAttributeDescriptor* attribDesc )
{
    PG_ASSERT( 0 <= numBinding && numBinding <= 8 );
    VertexInputDescriptor desc;
    for ( uint8_t i = 0; i < numBinding; ++i )
    {
        desc.m_vkBindingDescs[i].binding   = bindingDesc[i].binding;
        desc.m_vkBindingDescs[i].stride    = bindingDesc[i].stride;
        desc.m_vkBindingDescs[i].inputRate = PGToVulkanVertexInputRate( bindingDesc[i].inputRate );
    }

    for ( uint8_t i = 0; i < numAttrib; ++i )
    {
        desc.m_vkAttribDescs[i].location = attribDesc[i].location;
        desc.m_vkAttribDescs[i].binding  = attribDesc[i].binding;
        desc.m_vkAttribDescs[i].format   = PGToVulkanBufferFormat( attribDesc[i].format );
        desc.m_vkAttribDescs[i].offset   = attribDesc[i].offset;
    }

    desc.m_createInfo                                 = {};
    desc.m_createInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    desc.m_createInfo.vertexBindingDescriptionCount   = numBinding;
    desc.m_createInfo.vertexAttributeDescriptionCount = numAttrib;

    return desc;
}

const VkPipelineVertexInputStateCreateInfo& VertexInputDescriptor::GetHandle()
{
    m_createInfo.pVertexBindingDescriptions   = m_vkBindingDescs.data();
    m_createInfo.pVertexAttributeDescriptions = m_vkAttribDescs.data();
    return m_createInfo;
}

} // namespace Gfx
} // namespace PG
