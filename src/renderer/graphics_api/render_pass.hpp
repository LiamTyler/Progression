#pragma once

#include "renderer/graphics_api/texture.hpp"
#include "renderer/vulkan.hpp"
#include "glm/vec4.hpp"
#include <array>

namespace PG
{
namespace Gfx
{
    constexpr uint8_t MAX_COLOR_ATTACHMENTS = 8;

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

        void AddColorAttachment( 
            PixelFormat format = PixelFormat::INVALID,
            LoadAction loadAction = LoadAction::CLEAR,
            StoreAction storeAction = StoreAction::STORE,
            const glm::vec4& clearColor = glm::vec4( 0 ),
            ImageLayout initialLayout = ImageLayout::UNDEFINED,
            ImageLayout finalLayout = ImageLayout::UNDEFINED
        );
        void AddDepthAttachment(
            PixelFormat format = PixelFormat::INVALID,
            LoadAction loadAction = LoadAction::CLEAR,
            StoreAction storeAction = StoreAction::STORE,
            float clearValue = 1.0f,
            ImageLayout initialLayout = ImageLayout::UNDEFINED,
            ImageLayout finalLayout = ImageLayout::UNDEFINED
        );

        std::array< ColorAttachmentDescriptor, MAX_COLOR_ATTACHMENTS > colorAttachmentDescriptors;
        DepthAttachmentDescriptor depthAttachmentDescriptor;
        uint8_t numColorAttachments = 0;
        uint8_t numDepthAttachments = 0;
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

    std::string ImageLayoutToString( ImageLayout layout );

} // namespace Gfx
} // namespace PG
