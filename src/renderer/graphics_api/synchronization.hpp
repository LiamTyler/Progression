#pragma once

#include <vulkan/vulkan.h>

namespace PG
{
namespace Gfx
{

class Fence
{
    friend class Device;
public:
    Fence() = default;

    void Free();
    void WaitFor();
    void Reset();
    VkFence GetHandle() const;
    operator bool() const;

private:
    VkFence m_handle  = VK_NULL_HANDLE;
    VkDevice m_device;
};

class Semaphore
{
    friend class Device;
public:
    Semaphore() = default;

    void Free();
    VkSemaphore GetHandle() const;
    operator bool() const;

private:
    VkSemaphore m_handle = VK_NULL_HANDLE;
    VkDevice m_device;
};

} // namespace Gfx
} // namespace PG