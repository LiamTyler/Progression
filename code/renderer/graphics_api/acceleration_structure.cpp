#include "renderer/graphics_api/acceleration_structure.hpp"

namespace PG
{
namespace Gfx
{

    void AccelerationStructure::Free()
    {
#if USING( PG_RTX )
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        m_buffer.Free();
	    vkDestroyAccelerationStructureKHR( m_device, m_handle, nullptr );
#endif // #if USING( PG_RTX )
    }

} // namespace Gfx
} // namespace PG