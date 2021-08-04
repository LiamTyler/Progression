#include "renderer/debug_marker.hpp"
#include "utils/logger.hpp"
#include <cstring>
#include <vector>

namespace PG
{
namespace Gfx
{

namespace DebugMarker
{

	static bool s_active           = false;
	static bool s_extensionPresent = false;

	static PFN_vkSetDebugUtilsObjectTagEXT     vkSetDebugUtilsObjectTag  = VK_NULL_HANDLE;
	static PFN_vkSetDebugUtilsObjectNameEXT    vkSetDebugUtilsObjectName = VK_NULL_HANDLE;
	
	static PFN_vkCmdBeginDebugUtilsLabelEXT	   vkCmdDebugMarkerBegin     = VK_NULL_HANDLE;
	static PFN_vkCmdEndDebugUtilsLabelEXT	   vkCmdDebugMarkerEnd       = VK_NULL_HANDLE;
	static PFN_vkCmdInsertDebugUtilsLabelEXT   vkCmdDebugMarkerInsert    = VK_NULL_HANDLE;

	static PFN_vkQueueBeginDebugUtilsLabelEXT  vkQueueDebugMarkerBegin   = VK_NULL_HANDLE;
	static PFN_vkQueueEndDebugUtilsLabelEXT    vkQueueDebugMarkerEnd     = VK_NULL_HANDLE;
	static PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueDebugMarkerInsert  = VK_NULL_HANDLE;

	void Init( VkInstance instance )
	{
		vkSetDebugUtilsObjectTag  = (PFN_vkSetDebugUtilsObjectTagEXT)	  vkGetInstanceProcAddr( instance, "vkSetDebugUtilsObjectTagEXT" );
		vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)	  vkGetInstanceProcAddr( instance, "vkSetDebugUtilsObjectNameEXT" );
		vkCmdDebugMarkerBegin     = (PFN_vkCmdBeginDebugUtilsLabelEXT)    vkGetInstanceProcAddr( instance, "vkCmdBeginDebugUtilsLabelEXT" );
		vkCmdDebugMarkerEnd       = (PFN_vkCmdEndDebugUtilsLabelEXT)      vkGetInstanceProcAddr( instance, "vkCmdEndDebugUtilsLabelEXT" );
		vkCmdDebugMarkerInsert    = (PFN_vkCmdInsertDebugUtilsLabelEXT)   vkGetInstanceProcAddr( instance, "vkCmdInsertDebugUtilsLabelEXT" );
		vkQueueDebugMarkerBegin   = (PFN_vkQueueBeginDebugUtilsLabelEXT)  vkGetInstanceProcAddr( instance, "vkQueueBeginDebugUtilsLabelEXT" );
		vkQueueDebugMarkerEnd     = (PFN_vkQueueEndDebugUtilsLabelEXT)    vkGetInstanceProcAddr( instance, "vkQueueEndDebugUtilsLabelEXT" );
		vkQueueDebugMarkerInsert  = (PFN_vkQueueInsertDebugUtilsLabelEXT) vkGetInstanceProcAddr( instance, "vkQueueInsertDebugUtilsLabelEXT" );

		s_active = ( vkSetDebugUtilsObjectTag != VK_NULL_HANDLE && vkSetDebugUtilsObjectName != VK_NULL_HANDLE &&
			vkCmdDebugMarkerBegin != VK_NULL_HANDLE && vkCmdDebugMarkerEnd != VK_NULL_HANDLE && vkCmdDebugMarkerInsert != VK_NULL_HANDLE &&
			vkQueueDebugMarkerBegin != VK_NULL_HANDLE && vkQueueDebugMarkerEnd != VK_NULL_HANDLE && vkQueueDebugMarkerInsert != VK_NULL_HANDLE );
		if ( !s_active )
		{
			LOG_ERR( "Could not initialize debug marking functions!" );
		}

		s_active = false; // TODO: fix intermittent crashes when this is enabled
	}

    bool IsActive()
    {
        return s_active;
    }

	void SetObjectName( VkDevice device, uint64_t object, VkObjectType objectType, const char *name )
	{
		if ( s_active )
		{
			VkDebugUtilsObjectNameInfoEXT nameInfo;
			nameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			nameInfo.pNext		  = nullptr;
			nameInfo.objectType   = objectType;
			nameInfo.objectHandle = object;
			nameInfo.pObjectName  = name;
			VK_CHECK_RESULT( vkSetDebugUtilsObjectName( device, &nameInfo ) );
		}
	}

	void SetObjectTag( VkDevice device, uint64_t object, VkObjectType objectType, uint64_t name, size_t tagSize, const void* tag )
	{
		if ( s_active )
		{
			VkDebugUtilsObjectTagInfoEXT tagInfo;
			tagInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
			tagInfo.pNext		 = nullptr;
			tagInfo.objectType   = objectType;
			tagInfo.objectHandle = object;
			tagInfo.tagName      = name;
			tagInfo.tagSize      = tagSize;
			tagInfo.pTag         = tag;
			VK_CHECK_RESULT( vkSetDebugUtilsObjectTag( device, &tagInfo ) );
		}
	}

	void BeginRegion_CmdBuf( VkCommandBuffer cmdbuffer, const char* name, glm::vec4 color )
	{
		if ( s_active )
		{
			VkDebugUtilsLabelEXT label;
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pNext = nullptr;
			label.pLabelName = name;
			memcpy( label.color, &color[0], sizeof( float ) * 4 );
			vkCmdDebugMarkerBegin( cmdbuffer, &label );
		}
	}

	void Insert_CmdBuf( VkCommandBuffer cmdbuffer, const char* name, glm::vec4 color )
	{
		if ( s_active )
		{
			VkDebugUtilsLabelEXT label;
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pNext = nullptr;
			label.pLabelName = name;
			memcpy( label.color, &color[0], sizeof( float ) * 4 );
			vkCmdDebugMarkerInsert( cmdbuffer, &label );
		}
	}

	void EndRegion_CmdBuf( VkCommandBuffer cmdBuffer )
	{
		if ( s_active )
		{
			vkCmdDebugMarkerEnd( cmdBuffer );
		}
	}

	void BeginRegion_Queue( VkQueue queue, const char* name, glm::vec4 color )
	{
		if ( s_active )
		{
			VkDebugUtilsLabelEXT label;
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pNext = nullptr;
			label.pLabelName = name;
			memcpy( label.color, &color[0], sizeof( float ) * 4 );
			vkQueueDebugMarkerBegin( queue, &label );
		}
	}

	void Insert_Queue( VkQueue queue, const char* name, glm::vec4 color )
	{
		if ( s_active )
		{
			VkDebugUtilsLabelEXT label;
			label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			label.pNext = nullptr;
			label.pLabelName = name;
			memcpy( label.color, &color[0], sizeof( float ) * 4 );
			vkQueueDebugMarkerInsert( queue, &label );
		}
	}

	void EndRegion_Queue( VkQueue queue )
	{
		if ( s_active )
		{
			vkQueueDebugMarkerEnd( queue );
		}
	}

    void SetCommandPoolName( VkDevice device, VkCommandPool pool, const char * name )
    {
        SetObjectName( device, ( uint64_t )pool, VK_OBJECT_TYPE_COMMAND_POOL, name );
    }

    void SetCommandBufferName( VkDevice device, VkCommandBuffer cmdBuffer, const char * name )
    {
	    SetObjectName( device, ( uint64_t )cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, name );
    }

    void SetQueueName( VkDevice device, VkQueue queue, const char * name )
    {
	    SetObjectName( device, ( uint64_t )queue, VK_OBJECT_TYPE_QUEUE, name );
    }

    void SetImageName( VkDevice device, VkImage image, const char * name )
    {
	    SetObjectName( device, ( uint64_t )image, VK_OBJECT_TYPE_IMAGE, name );
    }

    void SetImageViewName( VkDevice device, VkImageView view, const char * name )
    {
	    SetObjectName( device, ( uint64_t )view, VK_OBJECT_TYPE_IMAGE_VIEW, name );
    }

    void SetSamplerName( VkDevice device, VkSampler sampler, const char * name )
    {
	    SetObjectName( device, ( uint64_t )sampler, VK_OBJECT_TYPE_SAMPLER, name );
    }

    void SetBufferName( VkDevice device, VkBuffer buffer, const char * name )
    {
	    SetObjectName( device, ( uint64_t )buffer, VK_OBJECT_TYPE_BUFFER, name );
    }

    void SetDeviceMemoryName( VkDevice device, VkDeviceMemory memory, const char * name )
    {
	    SetObjectName( device, ( uint64_t )memory, VK_OBJECT_TYPE_DEVICE_MEMORY, name );
    }

    void SetShaderModuleName( VkDevice device, VkShaderModule shaderModule, const char * name )
    {
	    SetObjectName( device, ( uint64_t )shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, name );
    }

    void SetPipelineName( VkDevice device, VkPipeline pipeline, const char * name )
    {
	    SetObjectName( device, ( uint64_t )pipeline, VK_OBJECT_TYPE_PIPELINE, name );
    }

    void SetPipelineLayoutName( VkDevice device, VkPipelineLayout pipelineLayout, const char * name )
    {
	    SetObjectName( device, ( uint64_t )pipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name );
    }

    void SetRenderPassName( VkDevice device, VkRenderPass renderPass, const char * name )
    {
	    SetObjectName( device, ( uint64_t )renderPass, VK_OBJECT_TYPE_RENDER_PASS, name );
    }

    void SetFramebufferName( VkDevice device, VkFramebuffer framebuffer, const char * name )
    {
	    SetObjectName( device, ( uint64_t )framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, name );
    }

    void SetDescriptorSetLayoutName( VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name )
    {
	    SetObjectName( device, ( uint64_t )descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, name );
    }

    void SetDescriptorSetName( VkDevice device, VkDescriptorSet descriptorSet, const char * name )
    {
	    SetObjectName( device, ( uint64_t )descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, name );
    }

    void SetSemaphoreName( VkDevice device, VkSemaphore semaphore, const char * name )
    {
	    SetObjectName( device, ( uint64_t )semaphore, VK_OBJECT_TYPE_SEMAPHORE, name );
    }

    void SetFenceName( VkDevice device, VkFence fence, const char * name )
    {
	    SetObjectName( device, ( uint64_t )fence, VK_OBJECT_TYPE_FENCE, name );
    }

    void SetEventName( VkDevice device, VkEvent _event, const char * name )
    {
	    SetObjectName( device, ( uint64_t )_event, VK_OBJECT_TYPE_EVENT, name );
    }

    void SetSwapChainName( VkDevice device, VkSwapchainKHR swapchain, const char * name )
    {
        SetObjectName( device, ( uint64_t )swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, name );
    }

    void SetPhysicalDeviceName( VkDevice device, VkPhysicalDevice pDev, const char * name )
    {
        SetObjectName( device, ( uint64_t )pDev, VK_OBJECT_TYPE_PHYSICAL_DEVICE, name );
    }

    void SetLogicalDeviceName( VkDevice device, const char * name )
    {
        SetObjectName( device, ( uint64_t )device, VK_OBJECT_TYPE_DEVICE, name );
    }

    void SetInstanceName( VkDevice device, VkInstance instance, const char * name )
    {
        SetObjectName( device, ( uint64_t )instance, VK_OBJECT_TYPE_INSTANCE, name );
    }

    void SetDescriptorPoolName( VkDevice device, VkDescriptorPool pool, const char * name )
    {
        SetObjectName( device, ( uint64_t )pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, name );
    }

	void SetQueryPoolName( VkDevice device, VkQueryPool pool, const char * name )
    {
        SetObjectName( device, ( uint64_t )pool, VK_OBJECT_TYPE_QUERY_POOL, name );
    }

} // namespace DebugMarker
} // namespace Gfx
} // namespace PG
