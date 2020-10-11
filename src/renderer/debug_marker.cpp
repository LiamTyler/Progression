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

	static PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag   = VK_NULL_HANDLE;
	static PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName = VK_NULL_HANDLE;
	static PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin           = VK_NULL_HANDLE;
	static PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd               = VK_NULL_HANDLE;
	static PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert         = VK_NULL_HANDLE;

	// Get function pointers for the debug report extensions from the device
	void Init( VkDevice device, VkPhysicalDevice physicalDevice )
	{
		// Check if the debug marker extension is present (which is the case if run from a graphics debugger)
		uint32_t extensionCount;
		VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, nullptr ) );
		std::vector< VkExtensionProperties > extensions( extensionCount );
		VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &extensionCount, extensions.data() ) );
		for ( auto extension : extensions )
        {
			if ( strcmp( extension.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0 )
            {
				s_extensionPresent = true;
				break;
			}
		}

		if ( s_extensionPresent )
        {
			// The debug marker extension is not part of the core, so function pointers need to be loaded manually
			vkDebugMarkerSetObjectTag   = (PFN_vkDebugMarkerSetObjectTagEXT)  vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectTagEXT" );
			vkDebugMarkerSetObjectName  = (PFN_vkDebugMarkerSetObjectNameEXT) vkGetDeviceProcAddr( device, "vkDebugMarkerSetObjectNameEXT" );
			vkCmdDebugMarkerBegin       = (PFN_vkCmdDebugMarkerBeginEXT)      vkGetDeviceProcAddr( device, "vkCmdDebugMarkerBeginEXT" );
			vkCmdDebugMarkerEnd         = (PFN_vkCmdDebugMarkerEndEXT)        vkGetDeviceProcAddr( device, "vkCmdDebugMarkerEndEXT" );
			vkCmdDebugMarkerInsert      = (PFN_vkCmdDebugMarkerInsertEXT)     vkGetDeviceProcAddr( device, "vkCmdDebugMarkerInsertEXT" );
			s_active = ( vkDebugMarkerSetObjectTag != VK_NULL_HANDLE && vkDebugMarkerSetObjectName != VK_NULL_HANDLE &&vkCmdDebugMarkerBegin != VK_NULL_HANDLE && vkCmdDebugMarkerEnd != VK_NULL_HANDLE && vkCmdDebugMarkerInsert != VK_NULL_HANDLE );
			if ( !s_active )
			{
				LOG_ERR( "Could not initialize vkDebugMarker extension!\n" );
			}
		}

        // Colored output is messed up in Renderdoc terminal
        if ( s_active )
        {
			Logger_ChangeLocationColored( "stdout", false );
        }
	}

    bool IsActive()
    {
        return s_active;
    }

	// Sets the debug name of an object
	// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
	// along with the object type
	void SetObjectName( VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name )
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if ( s_active )
		{
			VkDebugMarkerObjectNameInfoEXT nameInfo = {};
			nameInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
			nameInfo.objectType  = objectType;
			nameInfo.object      = object;
			nameInfo.pObjectName = name;
			VK_CHECK_RESULT( vkDebugMarkerSetObjectName( device, &nameInfo ) );
		}
	}

	// Set the tag for an object
	void SetObjectTag( VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag )
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if ( s_active )
		{
			VkDebugMarkerObjectTagInfoEXT tagInfo = {};
			tagInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT;
			tagInfo.objectType  = objectType;
			tagInfo.object      = object;
			tagInfo.tagName     = name;
			tagInfo.tagSize     = tagSize;
			tagInfo.pTag        = tag;
			VK_CHECK_RESULT( vkDebugMarkerSetObjectTag( device, &tagInfo ) );
		}
	}

	// Start a new debug marker region
	void BeginRegion( VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color )
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if ( s_active )
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
            memcpy( markerInfo.color, &color[0], sizeof( float ) * 4 );
			markerInfo.pMarkerName = pMarkerName;
			vkCmdDebugMarkerBegin( cmdbuffer, &markerInfo );
		}
	}

	// Insert a new debug marker into the command buffer
	void Insert( VkCommandBuffer cmdbuffer, const char* name, glm::vec4 color )
	{
		// Check for valid function pointer (may not be present if not running in a debugging application)
		if ( s_active )
		{
			VkDebugMarkerMarkerInfoEXT markerInfo = {};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
            memcpy( markerInfo.color, &color[0], sizeof( float ) * 4 );
			markerInfo.pMarkerName = name;
			vkCmdDebugMarkerInsert( cmdbuffer, &markerInfo );
		}
	}

	// End the current debug marker region
	void EndRegion( VkCommandBuffer cmdBuffer )
	{
		// Check for valid function (may not be present if not runnin in a debugging application)
		if ( vkCmdDebugMarkerEnd )
		{
			vkCmdDebugMarkerEnd( cmdBuffer );
		}
	}

    void SetCommandPoolName( VkDevice device, VkCommandPool pool, const char * name )
    {
        SetObjectName( device, ( uint64_t )pool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, name );
    }

    void SetCommandBufferName( VkDevice device, VkCommandBuffer cmdBuffer, const char * name )
    {
	    SetObjectName( device, ( uint64_t )cmdBuffer, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, name );
    }

    void SetQueueName( VkDevice device, VkQueue queue, const char * name )
    {
	    SetObjectName( device, ( uint64_t )queue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT, name );
    }

    void SetImageName( VkDevice device, VkImage image, const char * name )
    {
	    SetObjectName( device, ( uint64_t )image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name );
    }

    void SetImageViewName( VkDevice device, VkImageView view, const char * name )
    {
	    SetObjectName( device, ( uint64_t )view, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT, name );
    }

    void SetSamplerName( VkDevice device, VkSampler sampler, const char * name )
    {
	    SetObjectName( device, ( uint64_t )sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT, name );
    }

    void SetBufferName( VkDevice device, VkBuffer buffer, const char * name )
    {
	    SetObjectName( device, ( uint64_t )buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name );
    }

    void SetDeviceMemoryName( VkDevice device, VkDeviceMemory memory, const char * name )
    {
	    SetObjectName( device, ( uint64_t )memory, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, name );
    }

    void SetShaderModuleName( VkDevice device, VkShaderModule shaderModule, const char * name )
    {
	    SetObjectName( device, ( uint64_t )shaderModule, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, name );
    }

    void SetPipelineName( VkDevice device, VkPipeline pipeline, const char * name )
    {
	    SetObjectName( device, ( uint64_t )pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, name );
    }

    void SetPipelineLayoutName( VkDevice device, VkPipelineLayout pipelineLayout, const char * name )
    {
	    SetObjectName( device, ( uint64_t )pipelineLayout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, name );
    }

    void SetRenderPassName( VkDevice device, VkRenderPass renderPass, const char * name )
    {
	    SetObjectName( device, ( uint64_t )renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, name );
    }

    void SetFramebufferName( VkDevice device, VkFramebuffer framebuffer, const char * name )
    {
	    SetObjectName( device, ( uint64_t )framebuffer, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT, name );
    }

    void SetDescriptorSetLayoutName( VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name )
    {
	    SetObjectName( device, ( uint64_t )descriptorSetLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, name );
    }

    void SetDescriptorSetName( VkDevice device, VkDescriptorSet descriptorSet, const char * name )
    {
	    SetObjectName( device, ( uint64_t )descriptorSet, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, name );
    }

    void SetSemaphoreName( VkDevice device, VkSemaphore semaphore, const char * name )
    {
	    SetObjectName( device, ( uint64_t )semaphore, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT, name );
    }

    void SetFenceName( VkDevice device, VkFence fence, const char * name )
    {
	    SetObjectName( device, ( uint64_t )fence, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT, name );
    }

    void SetEventName( VkDevice device, VkEvent _event, const char * name )
    {
	    SetObjectName( device, ( uint64_t )_event, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT, name );
    }

    void SetSwapChainName( VkDevice device, VkSwapchainKHR swapchain, const char * name )
    {
        SetObjectName( device, ( uint64_t )swapchain, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT, name );
    }

    void SetPhysicalDeviceName( VkDevice device, VkPhysicalDevice pDev, const char * name )
    {
        SetObjectName( device, ( uint64_t )pDev, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, name );
    }

    void SetLogicalDeviceName( VkDevice device, const char * name )
    {
        SetObjectName( device, ( uint64_t )device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, name );
    }

    void SetInstanceName( VkDevice device, VkInstance instance, const char * name )
    {
        SetObjectName( device, ( uint64_t )instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, name );
    }

    void SetDescriptorPoolName( VkDevice device, VkDescriptorPool pool, const char * name )
    {
        SetObjectName( device, ( uint64_t )pool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, name );
    }

	void SetQueryPoolName( VkDevice device, VkQueryPool pool, const char * name )
    {
        SetObjectName( device, ( uint64_t )pool, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, name );
    }

} // namespace DebugMarker
} // namespace Gfx
} // namespace PG
