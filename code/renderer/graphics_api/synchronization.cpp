#include "renderer/graphics_api/synchronization.hpp"
#include "core/cpu_profiling.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"

namespace PG::Gfx
{

void Fence::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkDestroyFence( rg.device, m_handle, nullptr );
}

void Fence::WaitFor()
{
    PGP_ZONE_SCOPEDN( "Fence::WaitFor" );
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VK_CHECK( vkWaitForFences( rg.device, 1, &m_handle, VK_TRUE, UINT64_MAX ) );
}

void Fence::Reset()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VK_CHECK( vkResetFences( rg.device, 1, &m_handle ) );
}

VkResult Fence::GetStatus() const { return vkGetFenceStatus( rg.device, m_handle ); }

VkFence Fence::GetHandle() const { return m_handle; }

Fence::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Fence::operator VkFence() const { return m_handle; }

void Semaphore::Free() { vkDestroySemaphore( rg.device, m_handle, nullptr ); }

// cleanup dangerous semaphore with signal pending from vkAcquireNextImageKHR (tie it to a specific queue)
// https://github.com/KhronosGroup/Vulkan-Docs/issues/1059
void Semaphore::Unsignal() const
{
    const VkPipelineStageFlags psw = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores    = &m_handle;
    submitInfo.pWaitDstStageMask  = &psw;

    vkQueueSubmit( rg.device.GetQueue( QueueType::GRAPHICS ), 1, &submitInfo, VK_NULL_HANDLE );
}

VkSemaphore Semaphore::GetHandle() const { return m_handle; }

Semaphore::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Semaphore::operator VkSemaphore() const { return m_handle; }

} // namespace PG::Gfx
