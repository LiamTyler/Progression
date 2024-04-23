#include "renderer/graphics_api/synchronization.hpp"
#include "shared/assert.hpp"

namespace PG
{
namespace Gfx
{


    void Fence::Free()
    {
        PG_ASSERT( m_device != VK_NULL_HANDLE );
        vkDestroyFence( m_device, m_handle, nullptr );
    }


    void Fence::WaitFor()
    {
        PG_ASSERT( m_device != VK_NULL_HANDLE && m_handle != VK_NULL_HANDLE );
        VK_CHECK( vkWaitForFences( m_device, 1, &m_handle, VK_TRUE, UINT64_MAX ) );
    }


    void Fence::Reset()
    {
        PG_ASSERT( m_device != VK_NULL_HANDLE && m_handle != VK_NULL_HANDLE );
        VK_CHECK( vkResetFences( m_device, 1, &m_handle ) );
    }


    VkFence Fence::GetHandle() const
    {
        return m_handle;
    }


    Fence::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


    void Semaphore::Free()
    {
        vkDestroySemaphore( m_device, m_handle, nullptr );
    }
    

    VkSemaphore Semaphore::GetHandle() const
    {
        return m_handle;
    }


    Semaphore::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


} // namespace Gfx
} // namespace PG