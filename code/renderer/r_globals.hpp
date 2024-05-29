#pragma once

#include "graphics_api/device.hpp"
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

    Buffer objectModelMatricesBuffer;
    Buffer objectNormalMatricesBuffer;
    Buffer sceneGlobalsBuffer;
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
    uint32_t currentFrameIdx; // [0, NUM_FRAME_OVERLAP)
    uint32_t totalFrameCount;
    uint32_t sceneWidth;
    uint32_t sceneHeight;
    uint32_t displayWidth;
    uint32_t displayHeight;

    CommandPool immediateCmdPool;
    CommandBuffer immediateCmdBuffer;
    Fence immediateFence;

    FrameData& GetFrameData() { return frameData[currentFrameIdx]; }

    void EndFrame()
    {
        ++totalFrameCount;
        currentFrameIdx = ( currentFrameIdx + 1 ) % NUM_FRAME_OVERLAP;
    }

    template <typename Func>
    void ImmediateSubmit( Func func )
    {
        immediateFence.Reset();
        immediateCmdBuffer.Reset();
        immediateCmdBuffer.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
        func( immediateCmdBuffer );
        immediateCmdBuffer.EndRecording();
        SubmitImmediateCommandBuffer();
    }

private:
    void SubmitImmediateCommandBuffer();
};

Viewport DisplaySizedViewport( bool vulkanFlipViewport = true );
Scissor DisplaySizedScissor();
Viewport SceneSizedViewport( bool vulkanFlipViewport = true );
Scissor SceneSizedScissor();
VkDescriptorSetLayout GetEmptyDescriptorSetLayout();

extern R_Globals rg;

} // namespace PG::Gfx
