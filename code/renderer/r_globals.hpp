#pragma once

#include "graphics_api/device.hpp"
#include "graphics_api/framebuffer.hpp"
#include "graphics_api/swapchain.hpp"
#include "graphics_api/physical_device.hpp"

namespace PG
{
namespace Gfx
{

enum : unsigned char
{
    GFX_CMD_POOL_GRAPHICS,
    GFX_CMD_POOL_COMPUTE,
    GFX_CMD_POOL_TRANSIENT,

    GFX_CMD_POOL_TOTAL
};

struct R_Globals
{
    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    PhysicalDevice physicalDevice;
    SwapChain swapchain;
    uint32_t swapChainImageIndex;
    Framebuffer swapchainFramebuffers[GFX_MAX_SWAPCHAIN_IMAGES];
    CommandPool commandPools[GFX_CMD_POOL_TOTAL];
    DescriptorPool descriptorPool;
    CommandBuffer graphicsCommandBuffer;
    CommandBuffer computeCommandBuffer;
    Semaphore presentCompleteSemaphore;
    Semaphore renderCompleteSemaphore;
    Fence computeFence;
    bool headless;
    uint32_t sceneWidth;
    uint32_t sceneHeight;
};

Viewport DisplaySizedViewport( bool vulkanFlipViewport = true );
Scissor  DisplaySizedScissor();
Viewport SceneSizedViewport( bool vulkanFlipViewport = true );
Scissor  SceneSizedScissor();
VkDescriptorSetLayout GetEmptyDescriptorSetLayout();

extern R_Globals r_globals;

} // namespace Gfx
} // namespace PG