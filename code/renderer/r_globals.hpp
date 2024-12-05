#pragma once

#include "graphics_api/device.hpp"
#include "graphics_api/physical_device.hpp"
#include "graphics_api/swapchain.hpp"

#if USING( DEVELOPMENT_BUILD )
#include "core/frustum.hpp"
#endif // #if USING( DEVELOPMENT_BUILD )

namespace PG::Gfx
{

enum : u8
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

    Buffer meshDrawDataBuffer;
    Buffer meshCullData;
    Buffer modelMatricesBuffer;
    Buffer modelDequantizationBuffer;
    Buffer sceneGlobalsBuffer;
    u32 numMeshes;
};

struct R_Globals
{
    VkInstance instance;
    VkSurfaceKHR surface;
    Device device;
    PhysicalDevice physicalDevice;
    Swapchain swapchain;
    u32 swapchainImageIndex;
    FrameData frameData[NUM_FRAME_OVERLAP];
    u32 currentFrameIdx; // [0, NUM_FRAME_OVERLAP)
    u32 totalFrameCount;
    u32 sceneWidth;
    u32 sceneHeight;
    u32 displayWidth;
    u32 displayHeight;
    bool headless;

    CommandPool immediateCmdPool;
    CommandBuffer immediateCmdBuffer;
    Fence immediateFence;

#if USING( DEVELOPMENT_BUILD )
    Frustum debugCullingFrustum;
    vec3 debugCullingCameraPos;
#endif // #if USING( DEVELOPMENT_BUILD )

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

extern R_Globals rg;

} // namespace PG::Gfx
