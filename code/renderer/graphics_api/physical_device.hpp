#pragma once

#include "asset/types/shader.hpp"
#include "renderer/graphics_api/features.hpp"
#include "renderer/vulkan.hpp"
#include <memory>
#include <string>

namespace PG::Gfx
{

static constexpr u32 INVALID_QUEUE_FAMILY = ~0u;

struct PhysicalDeviceMetadata
{
    u32 mainQueueFamilyIndex                        = INVALID_QUEUE_FAMILY;
    u32 transferQueueFamilyIndex                    = INVALID_QUEUE_FAMILY;
    int suitabilityScore                            = 0;
    PhysicalDeviceExtensions extensions             = {};
    PhysicalDeviceFeatures features                 = {};
    VkPhysicalDeviceVulkan11Properties properties11 = {};
    VkPhysicalDeviceProperties properties           = {};
    VkPhysicalDeviceMemoryProperties memProperties  = {};
#if USING( PG_DESCRIPTOR_BUFFER )
    VkPhysicalDeviceDescriptorBufferPropertiesEXT dbProps = {};
#endif // #if USING( PG_DESCRIPTOR_BUFFER )
};

class PhysicalDevice
{
public:
    PhysicalDevice() = default;

    bool Init( VkPhysicalDevice pDev );

    void LogReasonsForInsuitability() const;
    bool IsSpirvShaderExtensionSupported( const std::string& extension ) const;
    bool IsSpirvShaderCapabilitySupported( ShaderStage stage, i32 capability ) const;

    std::string GetName() const;
    VkPhysicalDevice GetHandle() const;
    operator bool() const;
    operator VkPhysicalDevice() const;
    const PhysicalDeviceMetadata* GetMetadata() const;

    bool AreMeshShadersSupported() const;

private:
    void CalculateSuitabilityScore();

    VkPhysicalDevice m_handle = VK_NULL_HANDLE;
    std::shared_ptr<PhysicalDeviceMetadata> m_metadata;

    // cached values for faster access
    bool m_cachedMeshShadersSupported;
};

} // namespace PG::Gfx
