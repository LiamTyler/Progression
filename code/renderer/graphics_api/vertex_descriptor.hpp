#pragma once

#include "renderer/graphics_api/buffer.hpp"
#include <array>

namespace PG
{
namespace Gfx
{

    enum class VertexInputRate
    {
        PER_VERTEX   = 0,
        PER_INSTANCE = 1
    };

    class VertexBindingDescriptor
    {
    public:
        VertexBindingDescriptor() = default;
        VertexBindingDescriptor( uint32_t _binding, uint32_t _stride, VertexInputRate _inputRate = VertexInputRate::PER_VERTEX );
        uint32_t binding;
        uint32_t stride;
        VertexInputRate inputRate;
    };

    class VertexAttributeDescriptor
    {
    public:
        VertexAttributeDescriptor() = default;
        VertexAttributeDescriptor( uint32_t loc, uint32_t bind, BufferDataType _format, uint32_t _offset );
        uint32_t location;
        uint32_t binding;
        BufferDataType format;
        uint32_t offset;
    };

    class VertexInputDescriptor
    {
    public:
        VertexInputDescriptor() = default;

        static VertexInputDescriptor Create( uint8_t numBinding, VertexBindingDescriptor* bindingDesc,
                                             uint8_t numAttrib, VertexAttributeDescriptor* attribDesc );

        const VkPipelineVertexInputStateCreateInfo& GetHandle();

    private:
        VkPipelineVertexInputStateCreateInfo m_createInfo;
        std::array< VkVertexInputBindingDescription, 8 > m_vkBindingDescs;
        std::array< VkVertexInputAttributeDescription, 8 > m_vkAttribDescs;
    };

} // namespace Gfx
} // namespace PG