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

#define MAX_FRAMES_IN_FLIGHT 2

struct FrameData
{
    Buffer sceneConstantBuffer;
    Buffer lightBuffer;
    DescriptorSet sceneConstantDescSet;
    DescriptorSet skyboxDescSet;
    DescriptorSet lightsDescSet;
    DescriptorSet lightingAuxTexturesDescSet;
    DescriptorSet postProcessDescSet;

    CommandBuffer graphicsCmdBuf;
    Fence inFlightFence;
    Semaphore swapchainImgAvailableSemaphore;
    Semaphore renderCompleteSemaphore;
};

struct R_Globals
{
    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    PhysicalDevice physicalDevice;
    Swapchain swapchain;
    uint32_t swapchainImageIndex;
    CommandPool commandPools[GFX_CMD_POOL_TOTAL];
    DescriptorPool descriptorPool;
    FrameData frameData[MAX_FRAMES_IN_FLIGHT];
    uint32_t currentFrame;
    bool headless;
    uint32_t sceneWidth;
    uint32_t sceneHeight;
};

Viewport DisplaySizedViewport( bool vulkanFlipViewport = true );
Scissor DisplaySizedScissor();
Viewport SceneSizedViewport( bool vulkanFlipViewport = true );
Scissor SceneSizedScissor();
VkDescriptorSetLayout GetEmptyDescriptorSetLayout();

extern R_Globals rg;

} // namespace PG::Gfx
