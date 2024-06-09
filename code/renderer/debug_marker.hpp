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

void SetObjectName( VkDevice device, u64 object, VkObjectType objectType, std::string_view name );
void SetObjectTag( VkDevice device, u64 object, VkObjectType objectType, u64 name, size_t tagSize, const void* tag );

void BeginRegion_CmdBuf( VkCommandBuffer cmdbuffer, std::string_view name, vec4 color = vec4( 0 ) );
void Insert_CmdBuf( VkCommandBuffer cmdbuffer, vec4 color, const char* fmt, ... );
void EndRegion_CmdBuf( VkCommandBuffer cmdBuffer );

void BeginRegion_Queue( VkQueue queue, std::string_view name, vec4 color = vec4( 0 ) );
void Insert_Queue( VkQueue queue, std::string_view name, vec4 color = vec4( 0 ) );
void EndRegion_Queue( VkQueue queue );

// Object specific naming functions
void SetCommandPoolName( VkCommandPool pool, std::string_view name );
void SetCommandBufferName( VkCommandBuffer cmdBuffer, std::string_view name );
void SetQueueName( VkQueue queue, std::string_view name );
void SetImageName( VkImage image, std::string_view name );
void SetImageViewName( VkImageView image, std::string_view name );
void SetSamplerName( VkSampler sampler, std::string_view name );
void SetBufferName( VkBuffer buffer, std::string_view name );
void SetDeviceMemoryName( VkDeviceMemory memory, std::string_view name );
void SetShaderModuleName( VkShaderModule shaderModule, std::string_view name );
void SetPipelineName( VkPipeline pipeline, std::string_view name );
void SetPipelineLayoutName( VkPipelineLayout pipelineLayout, std::string_view name );
void SetDescriptorSetLayoutName( VkDescriptorSetLayout descriptorSetLayout, std::string_view name );
void SetDescriptorSetName( VkDescriptorSet descriptorSet, std::string_view name );
void SetSemaphoreName( VkSemaphore semaphore, std::string_view name );
void SetFenceName( VkFence fence, std::string_view name );
void SetEventName( VkEvent event, std::string_view name );
void SetSwapChainName( VkSwapchainKHR swapchain, std::string_view name );
void SetPhysicalDeviceName( VkPhysicalDevice pDev, std::string_view name ); // stops application in renderdoc?
void SetLogicalDeviceName( VkDevice device, std::string_view name );
void SetInstanceName( VkInstance instance, std::string_view name );
void SetDescriptorPoolName( VkDescriptorPool pool, std::string_view name );
void SetQueryPoolName( VkQueryPool pool, std::string_view name );

} // namespace PG::Gfx::DebugMarker

#if !USING( SHIP_BUILD ) && USING( GPU_DATA )

#define PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdbuf, name ) \
    PG::Gfx::DebugMarker::BeginRegion_CmdBuf( cmdbuf, name, PG::Gfx::DebugMarker::GetNextRegionColor() )
#define PG_DEBUG_MARKER_END_REGION_CMDBUF( cmdbuf ) PG::Gfx::DebugMarker::EndRegion_CmdBuf( cmdbuf )
#define PG_DEBUG_MARKER_INSERT_CMDBUF( cmdbuf, fmt, ... ) PG::Gfx::DebugMarker::Insert_CmdBuf( cmdbuf, vec4( 0 ), fmt, __VA_ARGS__ )

#define PG_DEBUG_MARKER_BEGIN_REGION_QUEUE( queue, name, color ) PG::Gfx::DebugMarker::BeginRegion_Queue( cmdbuf, name, color )
#define PG_DEBUG_MARKER_END_REGION_QUEUE( queue ) PG::Gfx::DebugMarker::EndRegion_Queue( cmdbuf )
#define PG_DEBUG_MARKER_INSERT_QUEUE( queue, name, color ) PG::Gfx::DebugMarker::Insert_Queue( cmdbuf, name, color )

#define PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name ) PG::Gfx::DebugMarker::SetBufferName( buffer, name )

#define PG_DEBUG_MARKER_SET_PIPELINE_NAME( pipeline, name ) PG::Gfx::DebugMarker::SetPipelineName( pipeline, name )
#define PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( pipeline, name ) PG::Gfx::DebugMarker::SetPipelineLayoutName( pipeline, name )

#define PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( pool, name ) PG::Gfx::DebugMarker::SetCommandPoolName( pool, name )
  // Todo: seems like setting the command buffer name has has no effect in renderdoc
#define PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( cmdbuf, name ) PG::Gfx::DebugMarker::SetCommandBufferName( cmdbuf, name )
#define PG_DEBUG_MARKER_SET_QUEUE_NAME( queue, name ) PG::Gfx::DebugMarker::SetQueueName( queue, name )
#define PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( view, name ) PG::Gfx::DebugMarker::SetImageViewName( view, name )
#define PG_DEBUG_MARKER_SET_IMAGE_NAME( img, name ) PG::Gfx::DebugMarker::SetImageName( img, name )
#define PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, name ) PG::Gfx::DebugMarker::SetSamplerName( sampler, name )
#define PG_DEBUG_MARKER_SET_MEMORY_NAME( memory, name ) PG::Gfx::DebugMarker::SetDeviceMemoryName( memory, name )
#define PG_DEBUG_MARKER_SET_SHADER_NAME( shader, name ) PG::Gfx::DebugMarker::SetShaderModuleName( shader, name )
#define PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( layout, name ) PG::Gfx::DebugMarker::SetDescriptorSetLayoutName( layout, name )
#define PG_DEBUG_MARKER_SET_DESC_SET_NAME( set, name ) PG::Gfx::DebugMarker::SetDescriptorSetName( set, name )
#define PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( semaphore, name ) PG::Gfx::DebugMarker::SetSemaphoreName( semaphore, name )
#define PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name ) PG::Gfx::DebugMarker::SetFenceName( fence, name )
#define PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( swapchain, name ) PG::Gfx::DebugMarker::SetSwapChainName( swapchain, name )
#define PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( pDev, name ) PG::Gfx::DebugMarker::SetPhysicalDeviceName( pDev, name )
#define PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( dev, name ) PG::Gfx::DebugMarker::SetLogicalDeviceName( dev, name )
#define PG_DEBUG_MARKER_SET_INSTANCE_NAME( instance, name ) PG::Gfx::DebugMarker::SetInstanceName( instance, name )
#define PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name ) PG::Gfx::DebugMarker::SetDescriptorPoolName( pool, name )
#define PG_DEBUG_MARKER_SET_QUERY_POOL_NAME( pool, name ) PG::Gfx::DebugMarker::SetQueryPoolName( pool, name )

#else // #if !USING( SHIP_BUILD ) && USING( GPU_DATA )

#define PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdbuf, name, color )
#define PG_DEBUG_MARKER_END_REGION_CMDBUF( cmdbuf )
#define PG_DEBUG_MARKER_INSERT_CMDBUF( cmdbuf, name )

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
