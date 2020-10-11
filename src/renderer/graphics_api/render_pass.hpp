#pragma once

#include "renderer/graphics_api/texture.hpp"
#include "renderer/vulkan.hpp"
#include "glm/vec4.hpp"
#include <array>

namespace PG
{
namespace Gfx
{

    enum class LoadAction
    {
        LOAD      = 0,
        CLEAR     = 1,
        DONT_CARE = 2,

        NUM_LOAD_ACTION
    };

    enum class StoreAction
    {
        STORE = 0,
        DONT_CARE = 1,

        NUM_STORE_ACTION
    }; 
    
    enum class ImageLayout
    {
        UNDEFINED = 0,
        GENERAL = 1,
        COLOR_ATTACHMENT_OPTIMAL = 2,
        DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
        DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
        SHADER_READ_ONLY_OPTIMAL = 5,
        TRANSFER_SRC_OPTIMAL = 6,
        TRANSFER_DST_OPTIMAL = 7,
        PREINITIALIZED = 8,
        DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
        DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
        PRESENT_SRC_KHR = 1000001002,

        NUM_IMAGE_LAYOUT
    };

    class ColorAttachmentDescriptor
    {
    public:
        ColorAttachmentDescriptor() = default;

        glm::vec4 clearColor      = glm::vec4( 0 );
        LoadAction loadAction     = LoadAction::CLEAR;
        StoreAction storeAction   = StoreAction::STORE;
        PixelFormat format        = PixelFormat::INVALID;
        ImageLayout initialLayout = ImageLayout::UNDEFINED;
        ImageLayout finalLayout   = ImageLayout::UNDEFINED;
    };

    class DepthAttachmentDescriptor
    {
    public:
        DepthAttachmentDescriptor() = default;

        float clearValue          = 1.0f;
        LoadAction loadAction     = LoadAction::CLEAR;
        StoreAction storeAction   = StoreAction::STORE;
        PixelFormat format        = PixelFormat::INVALID;
        ImageLayout initialLayout = ImageLayout::UNDEFINED;
        ImageLayout finalLayout   = ImageLayout::UNDEFINED;
    };

    class RenderPassDescriptor
    {
    public:
        RenderPassDescriptor() = default;

        std::array< ColorAttachmentDescriptor, 8 > colorAttachmentDescriptors;
        DepthAttachmentDescriptor depthAttachmentDescriptor;
    };

    class RenderPass
    {
        friend class Device;
    public:
        RenderPass() = default;

        void Free();
        VkRenderPass GetHandle() const;
        operator bool() const;

        RenderPassDescriptor desc;

    private:
        VkRenderPass m_handle = VK_NULL_HANDLE;
        VkDevice     m_device = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace PG
