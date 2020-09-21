#pragma once

#include "renderer/graphics_api.hpp"
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace PG
{
namespace Gfx
{
    const int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = ~0u;
        uint32_t presentFamily  = ~0u;
        uint32_t computeFamily  = ~0u;

        bool IsComplete() const
        {
            return graphicsFamily != ~0u && presentFamily != ~0u && computeFamily != ~0u;
        }
    };

    struct PhysicalDeviceInfo
    {
        VkPhysicalDevice device;
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        VkPhysicalDeviceMemoryProperties memProperties;
        std::string name;
        int score;
        QueueFamilyIndices indices;
        std::vector< std::string > availableExtensions;

        bool ExtensionSupported( const std::string& extensionName ) const;
    };

    struct SwapChain
    {
        bool Create( VkDevice device );
        uint32_t AcquireNextImage( const Semaphore& presentCompleteSemaphore );

        VkDevice device;
        VkSwapchainKHR swapChain;
        VkFormat imageFormat;
        VkExtent2D extent;
        uint32_t currentImage;
        std::vector< VkImage > images;
        std::vector< VkImageView > imageViews;
    };

    struct RenderState
    {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        PhysicalDeviceInfo physicalDeviceInfo;
        SwapChain swapChain;
        Texture depthTex;
        std::vector< Framebuffer > swapChainFramebuffers;
        CommandPool graphicsCommandPool;
        CommandPool transientCommandPool;
        CommandPool computeCommandPool;
        CommandBuffer graphicsCommandBuffer;
        CommandBuffer computeCommandBuffer;
        
        Semaphore presentCompleteSemaphore;
        Semaphore renderCompleteSemaphore;
        Fence computeFence;

        Device device;
        RenderPass renderPass;
    };

    extern RenderState g_renderState;

    bool VulkanInit();

    void VulkanShutdown();

    uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );

    void TransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layers = 1 );

    bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport );

    VkImageView CreateImageView( VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1, uint32_t layers = 1 );

} // namepspace Gfx
} // namepspace PG
