#pragma once

#include "graphics_api/descriptor.hpp"
#include "graphics_api/device.hpp"
#include "graphics_api/framebuffer.hpp"
#include "graphics_api/physical_device.hpp"
#include "graphics_api/swapchain.hpp"

namespace PG::Gfx
{

enum : unsigned char
{
    GFX_CMD_POOL_GRAPHICS,
    GFX_CMD_POOL_TRANSIENT,

    GFX_CMD_POOL_TOTAL
};

#define NUM_FRAME_OVERLAP 2

struct FrameData
{
    Semaphore swapchainSemaphore, renderingCompleteSemaphore;
    Fence renderingCompleteFence;

    CommandPool cmdPool;
    CommandBuffer primaryCmdBuffer;
};

struct R_Globals
{
    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    PhysicalDevice physicalDevice;
    Swapchain swapchain;
    uint32_t swapchainImageIndex;
    FrameData frameData[NUM_FRAME_OVERLAP];
    uint32_t currentFrameIdx;
    uint32_t sceneWidth;
    uint32_t sceneHeight;
    bool headless;

    FrameData& GetFrameData() { return frameData[currentFrameIdx % NUM_FRAME_OVERLAP]; }
};

Viewport DisplaySizedViewport( bool vulkanFlipViewport = true );
Scissor DisplaySizedScissor();
Viewport SceneSizedViewport( bool vulkanFlipViewport = true );
Scissor SceneSizedScissor();
VkDescriptorSetLayout GetEmptyDescriptorSetLayout();

extern R_Globals rg;

} // namespace PG::Gfx
