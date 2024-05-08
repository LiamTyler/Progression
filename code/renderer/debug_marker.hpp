#pragma once

#include "core/feature_defines.hpp"
#include "renderer/r_globals.hpp"
#include "shared/math_vec.hpp"
#include "vma/vk_mem_alloc.h"
#include <string>

namespace PG::Gfx::DebugMarker
{
// From https://www.saschawillems.de/blog/2016/05/28/tutorial-on-using-vulkans-vk_ext_debug_marker-with-renderdoc/

vec4 GetNextRegionColor();

// Get function pointers for the debug report extensions from the device
void Init( VkInstance instance );

bool IsActive();

void SetObjectName( VkDevice device, uint64_t object, VkObjectType objectType, const std::string& name );
void SetObjectTag( VkDevice device, uint64_t object, VkObjectType objectType, uint64_t name, size_t tagSize, const void* tag );

void BeginRegion_CmdBuf( VkCommandBuffer cmdbuffer, const std::string& name, vec4 color = vec4( 0 ) );
void Insert_CmdBuf( VkCommandBuffer cmdbuffer, const std::string& name, vec4 color = vec4( 0 ) );
void EndRegion_CmdBuf( VkCommandBuffer cmdBuffer );

void BeginRegion_Queue( VkQueue queue, const std::string& name, vec4 color = vec4( 0 ) );
void Insert_Queue( VkQueue queue, const std::string& name, vec4 color = vec4( 0 ) );
void EndRegion_Queue( VkQueue queue );

// Object specific naming functions
void SetCommandPoolName( VkDevice device, VkCommandPool pool, const std::string& name );
void SetCommandBufferName( VkDevice device, VkCommandBuffer cmdBuffer, const std::string& name );
void SetQueueName( VkDevice device, VkQueue queue, const std::string& name );
void SetImageName( VkDevice device, VkImage image, const std::string& name );
void SetImageViewName( VkDevice device, VkImageView image, const std::string& name );
void SetSamplerName( VkDevice device, VkSampler sampler, const std::string& name );
void SetBufferName( VkDevice device, VkBuffer buffer, const std::string& name );
void SetDeviceMemoryName( VkDevice device, VkDeviceMemory memory, const std::string& name );
void SetShaderModuleName( VkDevice device, VkShaderModule shaderModule, const std::string& name );
void SetPipelineName( VkDevice device, VkPipeline pipeline, const std::string& name );
void SetPipelineLayoutName( VkDevice device, VkPipelineLayout pipelineLayout, const std::string& name );
void SetRenderPassName( VkDevice device, VkRenderPass renderPass, const std::string& name );
void SetFramebufferName( VkDevice device, VkFramebuffer framebuffer, const std::string& name );
void SetDescriptorSetLayoutName( VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const std::string& name );
void SetDescriptorSetName( VkDevice device, VkDescriptorSet descriptorSet, const std::string& name );
void SetSemaphoreName( VkDevice device, VkSemaphore semaphore, const std::string& name );
void SetFenceName( VkDevice device, VkFence fence, const std::string& name );
void SetEventName( VkDevice device, VkEvent _event, const std::string& name );
void SetSwapChainName( VkDevice device, VkSwapchainKHR swapchain, const std::string& name );
void SetPhysicalDeviceName( VkDevice device, VkPhysicalDevice pDev, const std::string& name ); // stops application in renderdoc?
void SetLogicalDeviceName( VkDevice device, const std::string& name );
void SetInstanceName( VkDevice device, VkInstance instance, const std::string& name );
void SetDescriptorPoolName( VkDevice device, VkDescriptorPool pool, const std::string& name );
void SetQueryPoolName( VkDevice device, VkQueryPool pool, const std::string& name );

} // namespace PG::Gfx::DebugMarker

#if !USING( SHIP_BUILD ) && USING( GPU_DATA )

#define PG_DEBUG_MARKER_NAME( x, y ) ( std::string( x ) + y )
#define PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( s, x ) \
    if ( !s.empty() )                            \
    {                                            \
        x;                                       \
    }

#define PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdbuf, name, color ) \
    PG::Gfx::DebugMarker::BeginRegion_CmdBuf( cmdbuf, PG_DEBUG_MARKER_NAME( "", name ), color );
#define PG_DEBUG_MARKER_END_REGION_CMDBUF( cmdbuf ) PG::Gfx::DebugMarker::EndRegion_CmdBuf( cmdbuf );
#define PG_DEBUG_MARKER_INSERT_CMDBUF( cmdbuf, name, color ) \
    PG::Gfx::DebugMarker::Insert_CmdBuf( cmdbuf, PG_DEBUG_MARKER_NAME( "", name ), color );

#define PG_DEBUG_MARKER_BEGIN_REGION_QUEUE( queue, name, color ) \
    PG::Gfx::DebugMarker::BeginRegion_Queue( cmdbuf, PG_DEBUG_MARKER_NAME( "", name ), color );
#define PG_DEBUG_MARKER_END_REGION_QUEUE( queue ) PG::Gfx::DebugMarker::EndRegion_Queue( cmdbuf );
#define PG_DEBUG_MARKER_INSERT_QUEUE( queue, name, color ) \
    PG::Gfx::DebugMarker::Insert_Queue( cmdbuf, PG_DEBUG_MARKER_NAME( "", name ), color );

#define PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name ) \
    PG::Gfx::DebugMarker::SetBufferName( PG::Gfx::rg.device, ( buffer ).GetHandle(), PG_DEBUG_MARKER_NAME( "Buf: ", name ) );

#define PG_DEBUG_MARKER_SET_PIPELINE_NAME( pipeline, name )                                                                        \
    PG::Gfx::DebugMarker::SetPipelineName( PG::Gfx::rg.device, pipeline.GetHandle(), PG_DEBUG_MARKER_NAME( "Pipeline: ", name ) ); \
    PG::Gfx::DebugMarker::SetPipelineLayoutName(                                                                                   \
        PG::Gfx::rg.device, pipeline.GetLayoutHandle(), PG_DEBUG_MARKER_NAME( "PipelineLayout: ", name ) )

#define PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( pool, name ) \
    PG::Gfx::DebugMarker::SetCommandPoolName( PG::Gfx::rg.device, pool.GetHandle(), PG_DEBUG_MARKER_NAME( "CmdPool: ", name ) )
  // Todo: seems like setting the command buffer name has has no effect in renderdoc
#define PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( cmdbuf, name ) \
    PG::Gfx::DebugMarker::SetCommandBufferName( PG::Gfx::rg.device, cmdbuf.GetHandle(), PG_DEBUG_MARKER_NAME( "CmdBuf: ", name ) )
#define PG_DEBUG_MARKER_SET_QUEUE_NAME( queue, name ) \
    PG::Gfx::DebugMarker::SetQueueName( PG::Gfx::rg.device, queue, PG_DEBUG_MARKER_NAME( "Queue: ", name ) )
#define PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( view, name ) \
    PG::Gfx::DebugMarker::SetImageViewName( PG::Gfx::rg.device, view, PG_DEBUG_MARKER_NAME( "Img View: ", name ) )
#define PG_DEBUG_MARKER_SET_IMAGE_NAME( img, name ) \
    PG::Gfx::DebugMarker::SetImageName( PG::Gfx::rg.device, img, PG_DEBUG_MARKER_NAME( "Img: ", name ) )
#define PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, name ) \
    PG::Gfx::DebugMarker::SetSamplerName( PG::Gfx::rg.device, sampler.GetHandle(), PG_DEBUG_MARKER_NAME( "Sampler: ", name ) )
#define PG_DEBUG_MARKER_SET_MEMORY_NAME( memory, name ) \
    PG::Gfx::DebugMarker::SetDeviceMemoryName( PG::Gfx::rg.device, memory, PG_DEBUG_MARKER_NAME( "Memory: ", name ) )
#define PG_DEBUG_MARKER_SET_SHADER_NAME( shaderPtr, name ) \
    PG::Gfx::DebugMarker::SetShaderModuleName( PG::Gfx::rg.device, shaderPtr->handle, PG_DEBUG_MARKER_NAME( "Shader: ", name ) )
#define PG_DEBUG_MARKER_SET_RENDER_PASS_NAME( pass, name ) \
    PG::Gfx::DebugMarker::SetRenderPassName( PG::Gfx::rg.device, pass.GetHandle(), PG_DEBUG_MARKER_NAME( "RenderPass: ", name ) )
#define PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( framebuffer, name ) \
    PG::Gfx::DebugMarker::SetFramebufferName( PG::Gfx::rg.device, framebuffer.GetHandle(), PG_DEBUG_MARKER_NAME( "Framebuf: ", name ) )
#define PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( layout, name ) \
    PG::Gfx::DebugMarker::SetDescriptorSetLayoutName(            \
        PG::Gfx::rg.device, layout.GetHandle(), PG_DEBUG_MARKER_NAME( "DescSetLayout: ", name ) )
#define PG_DEBUG_MARKER_SET_DESC_SET_NAME( set, name ) \
    PG::Gfx::DebugMarker::SetDescriptorSetName( PG::Gfx::rg.device, set.GetHandle(), PG_DEBUG_MARKER_NAME( "DescSet: ", name ) )
#define PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( semaphore, name ) \
    PG::Gfx::DebugMarker::SetSemaphoreName( PG::Gfx::rg.device, semaphore.GetHandle(), PG_DEBUG_MARKER_NAME( "Semaphore: ", name ) )
#define PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name ) \
    PG::Gfx::DebugMarker::SetFenceName( PG::Gfx::rg.device, fence.GetHandle(), PG_DEBUG_MARKER_NAME( "Fence: ", name ) )
#define PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( swapchain, name ) \
    PG::Gfx::DebugMarker::SetSwapChainName( PG::Gfx::rg.device, swapchain, PG_DEBUG_MARKER_NAME( "Swapchain: ", name ) )
#define PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( pDev, name ) \
    PG::Gfx::DebugMarker::SetPhysicalDeviceName(               \
        PG::Gfx::rg.device, ( pDev ).GetHandle(), PG_DEBUG_MARKER_NAME( "Physical Device: ", name ) )
#define PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( dev, name ) \
    PG::Gfx::DebugMarker::SetLogicalDeviceName( dev, PG_DEBUG_MARKER_NAME( "Logical Device: ", name ) )
#define PG_DEBUG_MARKER_SET_INSTANCE_NAME( instance, name ) \
    PG::Gfx::DebugMarker::SetInstanceName( PG::Gfx::rg.device, instance, PG_DEBUG_MARKER_NAME( "Instance: ", name ) )
#define PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name ) \
    PG::Gfx::DebugMarker::SetDescriptorPoolName( PG::Gfx::rg.device, pool, PG_DEBUG_MARKER_NAME( "DescPool: ", name ) )
#define PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( pool, name ) \
    PG::Gfx::DebugMarker::SetQueryPoolName( PG::Gfx::rg.device, pool, PG_DEBUG_MARKER_NAME( "QueryPool: ", name ) )

#else // #if !USING( SHIP_BUILD ) && USING( GPU_DATA )

#define PG_DEBUG_MARKER_NAME( x, y )
#define PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( s, x )

#define PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdbuf, name, color )
#define PG_DEBUG_MARKER_END_REGION_CMDBUF( cmdbuf )
#define PG_DEBUG_MARKER_INSERT_CMDBUF( cmdbuf, name, color )

#define PG_DEBUG_MARKER_BEGIN_REGION_QUEUE( queue, name, color )
#define PG_DEBUG_MARKER_END_REGION_QUEUE( queue )
#define PG_DEBUG_MARKER_INSERT_QUEUE( queue, name, color )

#define PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( pool, name )
#define PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( cmdbuf, name )
#define PG_DEBUG_MARKER_SET_QUEUE_NAME( queue, name )
#define PG_DEBUG_MARKER_SET_IMAGE_NAME( image, name )
#define PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( view, name )
#define PG_DEBUG_MARKER_SET_IMAGE_ONLY_NAME( img, name )
#define PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, name )
#define PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name )
#define PG_DEBUG_MARKER_SET_MEMORY_NAME( memory, name )
#define PG_DEBUG_MARKER_SET_SHADER_NAME( shader, name )
#define PG_DEBUG_MARKER_SET_PIPELINE_NAME( pipeline, name )
#define PG_DEBUG_MARKER_SET_RENDER_PASS_NAME( pass, name )
#define PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( framebuffer, name )
#define PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( layout, name )
#define PG_DEBUG_MARKER_SET_DESC_SET_NAME( set, name )
#define PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( semaphore, name )
#define PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name )
#define PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( swapchain, name )
#define PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( pDev, name )
#define PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( dev, name )
#define PG_DEBUG_MARKER_SET_INSTANCE_NAME( instance, name )
#define PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name )
#define PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( pool, name )

#endif // #else // #if !USING( SHIP_BUILD ) && USING( GPU_DATA )
