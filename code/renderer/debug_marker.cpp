#include "renderer/debug_marker.hpp"
#include "shared/logger.hpp"
#include <cstdarg>
#include <cstring>
#include <vector>

static vec4 s_regionColors[] = {
    vec4( 213, 62, 79, 255 ) / 255.0f,
    vec4( 252, 141, 89, 255 ) / 255.0f,
    vec4( 254, 224, 139, 255 ) / 255.0f,
    vec4( 230, 245, 152, 255 ) / 255.0f,
    vec4( 153, 213, 148, 255 ) / 255.0f,
    vec4( 50, 136, 189, 255 ) / 255.0f,
};

struct NameBuilder
{
    static constexpr size_t MAX_NAME_LEN = 63;
    NameBuilder( std::string_view prefix, std::string_view name )
    {
        size_t prefixL = Min( prefix.length(), MAX_NAME_LEN );
        memcpy( data, prefix.data(), prefixL );
        size_t nameL = Min( name.length(), MAX_NAME_LEN - prefixL );
        memcpy( data + prefixL, name.data(), nameL );
        data[prefixL + nameL] = '\0';
    }

    std::string_view GetName() { return data; }

    operator std::string_view() const { return data; }

    char data[MAX_NAME_LEN + 1];
};

namespace PG::Gfx::DebugMarker
{

vec4 GetNextRegionColor()
{
    static size_t index = 0;
    return s_regionColors[index++ % ARRAY_COUNT( s_regionColors )];
}

static bool s_active           = false;
static bool s_extensionPresent = false;

static PFN_vkSetDebugUtilsObjectTagEXT vkSetDebugUtilsObjectTag   = VK_NULL_HANDLE;
static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectName = VK_NULL_HANDLE;

static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdDebugMarkerBegin   = VK_NULL_HANDLE;
static PFN_vkCmdEndDebugUtilsLabelEXT vkCmdDebugMarkerEnd       = VK_NULL_HANDLE;
static PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdDebugMarkerInsert = VK_NULL_HANDLE;

static PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueDebugMarkerBegin   = VK_NULL_HANDLE;
static PFN_vkQueueEndDebugUtilsLabelEXT vkQueueDebugMarkerEnd       = VK_NULL_HANDLE;
static PFN_vkQueueInsertDebugUtilsLabelEXT vkQueueDebugMarkerInsert = VK_NULL_HANDLE;

void Init( VkInstance instance )
{
#if USING( USE_DEBUG_MARKER )
    vkSetDebugUtilsObjectTag  = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr( instance, "vkSetDebugUtilsObjectTagEXT" );
    vkSetDebugUtilsObjectName = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr( instance, "vkSetDebugUtilsObjectNameEXT" );
    vkCmdDebugMarkerBegin     = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( instance, "vkCmdBeginDebugUtilsLabelEXT" );
    vkCmdDebugMarkerEnd       = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( instance, "vkCmdEndDebugUtilsLabelEXT" );
    vkCmdDebugMarkerInsert    = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( instance, "vkCmdInsertDebugUtilsLabelEXT" );
    vkQueueDebugMarkerBegin   = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( instance, "vkQueueBeginDebugUtilsLabelEXT" );
    vkQueueDebugMarkerEnd     = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( instance, "vkQueueEndDebugUtilsLabelEXT" );
    vkQueueDebugMarkerInsert  = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( instance, "vkQueueInsertDebugUtilsLabelEXT" );

    s_active = ( vkSetDebugUtilsObjectTag != VK_NULL_HANDLE && vkSetDebugUtilsObjectName != VK_NULL_HANDLE &&
                 vkCmdDebugMarkerBegin != VK_NULL_HANDLE && vkCmdDebugMarkerEnd != VK_NULL_HANDLE &&
                 vkCmdDebugMarkerInsert != VK_NULL_HANDLE && vkQueueDebugMarkerBegin != VK_NULL_HANDLE &&
                 vkQueueDebugMarkerEnd != VK_NULL_HANDLE && vkQueueDebugMarkerInsert != VK_NULL_HANDLE );
    if ( !s_active )
    {
        LOG_ERR( "Could not initialize debug marking functions!" );
    }
#endif // #if USING( USE_DEBUG_MARKER )
}

bool IsActive() { return s_active; }

void SetObjectName( VkDevice device, u64 object, VkObjectType objectType, std::string_view name )
{
    if ( s_active )
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo;
        nameInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.pNext        = nullptr;
        nameInfo.objectType   = objectType;
        nameInfo.objectHandle = object;
        nameInfo.pObjectName  = name.data();
        VK_CHECK( vkSetDebugUtilsObjectName( device, &nameInfo ) );
    }
}

void SetObjectTag( VkDevice device, u64 object, VkObjectType objectType, u64 name, size_t tagSize, const void* tag )
{
    if ( s_active )
    {
        VkDebugUtilsObjectTagInfoEXT tagInfo;
        tagInfo.sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_TAG_INFO_EXT;
        tagInfo.pNext        = nullptr;
        tagInfo.objectType   = objectType;
        tagInfo.objectHandle = object;
        tagInfo.tagName      = name;
        tagInfo.tagSize      = tagSize;
        tagInfo.pTag         = tag;
        VK_CHECK( vkSetDebugUtilsObjectTag( device, &tagInfo ) );
    }
}

void BeginRegion_CmdBuf( VkCommandBuffer cmdbuffer, std::string_view name, vec4 color )
{
    if ( s_active )
    {
        VkDebugUtilsLabelEXT label;
        label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext      = nullptr;
        label.pLabelName = name.data();
        memcpy( label.color, &color[0], sizeof( f32 ) * 4 );
        vkCmdDebugMarkerBegin( cmdbuffer, &label );
    }
}

void Insert_CmdBuf( VkCommandBuffer cmdbuffer, vec4 color, const char* fmt, ... )
{
    if ( s_active )
    {
        char nameBuffer[64];
        va_list args;
        va_start( args, fmt );
        vsnprintf( nameBuffer, sizeof( nameBuffer ), fmt, args );
        va_end( args );
        VkDebugUtilsLabelEXT label;
        label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext      = nullptr;
        label.pLabelName = nameBuffer;
        memcpy( label.color, &color[0], sizeof( f32 ) * 4 );
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

void BeginRegion_Queue( VkQueue queue, std::string_view name, vec4 color )
{
    if ( s_active )
    {
        VkDebugUtilsLabelEXT label;
        label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext      = nullptr;
        label.pLabelName = name.data();
        memcpy( label.color, &color[0], sizeof( f32 ) * 4 );
        vkQueueDebugMarkerBegin( queue, &label );
    }
}

void Insert_Queue( VkQueue queue, std::string_view name, vec4 color )
{
    if ( s_active )
    {
        VkDebugUtilsLabelEXT label;
        label.sType      = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        label.pNext      = nullptr;
        label.pLabelName = name.data();
        memcpy( label.color, &color[0], sizeof( f32 ) * 4 );
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

void SetCommandPoolName( VkCommandPool pool, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "CmdPool: ", name );
        SetObjectName( rg.device, (u64)pool, VK_OBJECT_TYPE_COMMAND_POOL, finalName );
    }
}

void SetCommandPoolName( VkCommandPool pool, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "CmdPool: ", name );
        SetObjectName( rg.device, (u64)pool, VK_OBJECT_TYPE_COMMAND_POOL, finalName );
    }
}

void SetCommandBufferName( VkCommandBuffer cmdBuffer, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "CmdBuf: ", name );
        SetObjectName( rg.device, (u64)cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, finalName );
    }
}

void SetCommandBufferName( VkCommandBuffer cmdBuffer, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "CmdBuf: ", name );
        SetObjectName( rg.device, (u64)cmdBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, finalName );
    }
}

void SetQueueName( VkQueue queue, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Queue: ", name );
        SetObjectName( rg.device, (u64)queue, VK_OBJECT_TYPE_QUEUE, finalName );
    }
}

void SetQueueName( VkQueue queue, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Queue: ", name );
        SetObjectName( rg.device, (u64)queue, VK_OBJECT_TYPE_QUEUE, finalName );
    }
}

void SetImageName( VkImage image, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Image: ", name );
        SetObjectName( rg.device, (u64)image, VK_OBJECT_TYPE_IMAGE, finalName );
    }
}

void SetImageName( VkImage image, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Image: ", name );
        SetObjectName( rg.device, (u64)image, VK_OBJECT_TYPE_IMAGE, finalName );
    }
}

void SetImageViewName( VkImageView view, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "ImageView: ", name );
        SetObjectName( rg.device, (u64)view, VK_OBJECT_TYPE_IMAGE_VIEW, finalName );
    }
}

void SetImageViewName( VkImageView view, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "ImageView: ", name );
        SetObjectName( rg.device, (u64)view, VK_OBJECT_TYPE_IMAGE_VIEW, finalName );
    }
}

void SetSamplerName( VkSampler sampler, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Sampler: ", name );
        SetObjectName( rg.device, (u64)sampler, VK_OBJECT_TYPE_SAMPLER, finalName );
    }
}

void SetSamplerName( VkSampler sampler, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Sampler: ", name );
        SetObjectName( rg.device, (u64)sampler, VK_OBJECT_TYPE_SAMPLER, finalName );
    }
}

void SetBufferName( VkBuffer buffer, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Buffer: ", name );
        SetObjectName( rg.device, (u64)buffer, VK_OBJECT_TYPE_BUFFER, finalName );
    }
}

void SetBufferName( VkBuffer buffer, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Buffer: ", name );
        SetObjectName( rg.device, (u64)buffer, VK_OBJECT_TYPE_BUFFER, finalName );
    }
}

void SetDeviceMemoryName( VkDeviceMemory memory, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Memory: ", name );
        SetObjectName( rg.device, (u64)memory, VK_OBJECT_TYPE_DEVICE_MEMORY, finalName );
    }
}

void SetDeviceMemoryName( VkDeviceMemory memory, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Memory: ", name );
        SetObjectName( rg.device, (u64)memory, VK_OBJECT_TYPE_DEVICE_MEMORY, finalName );
    }
}

void SetShaderModuleName( VkShaderModule shaderModule, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "ShaderModule: ", name );
        SetObjectName( rg.device, (u64)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, finalName );
    }
}

void SetShaderModuleName( VkShaderModule shaderModule, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "ShaderModule: ", name );
        SetObjectName( rg.device, (u64)shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, finalName );
    }
}

void SetPipelineName( VkPipeline pipeline, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Pipeline: ", name );
        SetObjectName( rg.device, (u64)pipeline, VK_OBJECT_TYPE_PIPELINE, finalName );
    }
}

void SetPipelineName( VkPipeline pipeline, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Pipeline: ", name );
        SetObjectName( rg.device, (u64)pipeline, VK_OBJECT_TYPE_PIPELINE, finalName );
    }
}

void SetPipelineLayoutName( VkPipelineLayout pipelineLayout, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "PipelineLayout: ", name );
        SetObjectName( rg.device, (u64)pipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, finalName );
    }
}

void SetPipelineLayoutName( VkPipelineLayout pipelineLayout, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "PipelineLayout: ", name );
        SetObjectName( rg.device, (u64)pipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, finalName );
    }
}

void SetDescriptorSetLayoutName( VkDescriptorSetLayout descriptorSetLayout, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "DescLayout: ", name );
        SetObjectName( rg.device, (u64)descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, finalName );
    }
}

void SetDescriptorSetLayoutName( VkDescriptorSetLayout descriptorSetLayout, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "DescLayout: ", name );
        SetObjectName( rg.device, (u64)descriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, finalName );
    }
}

void SetDescriptorSetName( VkDescriptorSet descriptorSet, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "DescSet: ", name );
        SetObjectName( rg.device, (u64)descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, finalName );
    }
}

void SetDescriptorSetName( VkDescriptorSet descriptorSet, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "DescSet: ", name );
        SetObjectName( rg.device, (u64)descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET, finalName );
    }
}

void SetSemaphoreName( VkSemaphore semaphore, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Semaphore: ", name );
        SetObjectName( rg.device, (u64)semaphore, VK_OBJECT_TYPE_SEMAPHORE, finalName );
    }
}

void SetSemaphoreName( VkSemaphore semaphore, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Semaphore: ", name );
        SetObjectName( rg.device, (u64)semaphore, VK_OBJECT_TYPE_SEMAPHORE, finalName );
    }
}

void SetFenceName( VkFence fence, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Fence: ", name );
        SetObjectName( rg.device, (u64)fence, VK_OBJECT_TYPE_FENCE, finalName );
    }
}

void SetFenceName( VkFence fence, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Fence: ", name );
        SetObjectName( rg.device, (u64)fence, VK_OBJECT_TYPE_FENCE, finalName );
    }
}

void SetEventName( VkEvent event, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Event: ", name );
        SetObjectName( rg.device, (u64)event, VK_OBJECT_TYPE_EVENT, finalName );
    }
}

void SetEventName( VkEvent event, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Event: ", name );
        SetObjectName( rg.device, (u64)event, VK_OBJECT_TYPE_EVENT, finalName );
    }
}

void SetSwapChainName( VkSwapchainKHR swapchain, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Swapchain: ", name );
        SetObjectName( rg.device, (u64)swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, finalName );
    }
}

void SetSwapChainName( VkSwapchainKHR swapchain, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Swapchain: ", name );
        SetObjectName( rg.device, (u64)swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, finalName );
    }
}

void SetPhysicalDeviceName( VkPhysicalDevice pDev, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "PhysicalDevice: ", name );
        SetObjectName( rg.device, (u64)pDev, VK_OBJECT_TYPE_PHYSICAL_DEVICE, finalName );
    }
}

void SetPhysicalDeviceName( VkPhysicalDevice pDev, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "PhysicalDevice: ", name );
        SetObjectName( rg.device, (u64)pDev, VK_OBJECT_TYPE_PHYSICAL_DEVICE, finalName );
    }
}

void SetLogicalDeviceName( VkDevice device, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "LogicalDevice: ", name );
        SetObjectName( device, (u64)device, VK_OBJECT_TYPE_DEVICE, finalName );
    }
}

void SetLogicalDeviceName( VkDevice device, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "LogicalDevice: ", name );
        SetObjectName( device, (u64)device, VK_OBJECT_TYPE_DEVICE, finalName );
    }
}

void SetInstanceName( VkInstance instance, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "Instance: ", name );
        SetObjectName( rg.device, (u64)instance, VK_OBJECT_TYPE_INSTANCE, finalName );
    }
}

void SetInstanceName( VkInstance instance, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "Instance: ", name );
        SetObjectName( rg.device, (u64)instance, VK_OBJECT_TYPE_INSTANCE, finalName );
    }
}

void SetDescriptorPoolName( VkDescriptorPool pool, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "DescPool: ", name );
        SetObjectName( rg.device, (u64)pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, finalName );
    }
}

void SetDescriptorPoolName( VkDescriptorPool pool, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "DescPool: ", name );
        SetObjectName( rg.device, (u64)pool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, finalName );
    }
}

void SetQueryPoolName( VkQueryPool pool, std::string_view name )
{
    if ( !name.empty() )
    {
        NameBuilder finalName( "QueryPool: ", name );
        SetObjectName( rg.device, (u64)pool, VK_OBJECT_TYPE_QUERY_POOL, finalName );
    }
}

void SetQueryPoolName( VkQueryPool pool, const char* name )
{
    if ( name )
    {
        NameBuilder finalName( "QueryPool: ", name );
        SetObjectName( rg.device, (u64)pool, VK_OBJECT_TYPE_QUERY_POOL, finalName );
    }
}

} // namespace PG::Gfx::DebugMarker
