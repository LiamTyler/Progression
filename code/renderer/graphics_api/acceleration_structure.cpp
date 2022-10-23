#include "renderer/graphics_api/acceleration_structure.hpp"

namespace PG
{
namespace Gfx
{

    void AccelerationStructure::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        m_buffer.Free();
	    vkDestroyAccelerationStructureKHR( m_device, m_handle, nullptr );
    }

} // namespace Gfx
} // namespace PG