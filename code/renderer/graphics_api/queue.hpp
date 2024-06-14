#pragma once

#include "renderer/vulkan.hpp"

namespace PG::Gfx
{

enum class QueueType : u8
{
    GRAPHICS = 0,
    TRANSFER = 1,

    COUNT = 2
};

struct Queue
{
    VkQueue queue   = VK_NULL_HANDLE;
    u32 familyIndex = ~0u;
    u32 queueIndex  = ~0u;

    operator VkQueue() const { return queue; }
};

} // namespace PG::Gfx
