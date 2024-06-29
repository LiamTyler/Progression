#pragma once

#include "renderer/vulkan.hpp"

namespace PG::Gfx
{

class Fence
{
    friend class Device;

public:
    Fence() = default;

    void Free();
    void WaitFor();
    void Reset();
    VkResult GetStatus() const;
    VkFence GetHandle() const;
    operator bool() const;
    operator VkFence() const;

private:
    VkFence m_handle = VK_NULL_HANDLE;
};

class Semaphore
{
    friend class Device;

public:
    Semaphore() = default;

    void Free();
    void Unsignal() const;
    VkSemaphore GetHandle() const;
    operator bool() const;
    operator VkSemaphore() const;

private:
    VkSemaphore m_handle = VK_NULL_HANDLE;
};

} // namespace PG::Gfx
