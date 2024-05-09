#include "renderer/graphics_api/synchronization.hpp"
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
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VK_CHECK( vkWaitForFences( rg.device, 1, &m_handle, VK_TRUE, UINT64_MAX ) );
}

void Fence::Reset()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    VK_CHECK( vkResetFences( rg.device, 1, &m_handle ) );
}

VkFence Fence::GetHandle() const { return m_handle; }

Fence::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Fence::operator VkFence() const { return m_handle; }

void Semaphore::Free() { vkDestroySemaphore( rg.device, m_handle, nullptr ); }

VkSemaphore Semaphore::GetHandle() const { return m_handle; }

Semaphore::operator bool() const { return m_handle != VK_NULL_HANDLE; }
Semaphore::operator VkSemaphore() const { return m_handle; }

} // namespace PG::Gfx
