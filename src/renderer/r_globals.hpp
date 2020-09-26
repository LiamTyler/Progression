#pragma once

#include "graphics_api/device.hpp"
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
    RenderPass renderPass;
    Device device;
    PhysicalDevice physicalDevice;
    SwapChain swapchain;
    Framebuffer swapchainFramebuffers[GFX_MAX_SWAPCHAIN_IMAGES];
    CommandPool commandPools[GFX_CMD_POOL_TOTAL];
    Texture depthTex;
    CommandBuffer graphicsCommandBuffer;
    CommandBuffer computeCommandBuffer;
    Semaphore presentCompleteSemaphore;
    Semaphore renderCompleteSemaphore;
    Fence computeFence;
};

extern R_Globals r_globals;

} // namespace Gfx
} // namespace PG