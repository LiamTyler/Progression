#include "renderer/graphics_api/buffer.hpp"
#include "core/assert.hpp"
#include "core/core_defines.hpp"

namespace PG
{
namespace Gfx
{

    int SizeOfIndexType( IndexType type )
    {
        int size[] =
        {
            2, // UNSIGNED_SHORT
            4, // UNSIGNED_INT
        };

        static_assert( ARRAY_COUNT( size ) == static_cast< int >( IndexType::NUM_INDEX_TYPE ) );

        return size[static_cast< int >( type )];
    }


    void Buffer::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyBuffer( m_device, m_handle, nullptr );
        vkFreeMemory( m_device, m_memory, nullptr );
        m_handle = VK_NULL_HANDLE;
    }


    void Buffer::Map()
    {
        vkMapMemory( m_device, m_memory, 0, VK_WHOLE_SIZE, 0, &m_mappedPtr );
    }


    void Buffer::UnMap()
    {
        vkUnmapMemory( m_device, m_memory );
        m_mappedPtr = nullptr;
    }


    void Buffer::BindMemory( size_t offset ) const
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE && m_memory != VK_NULL_HANDLE );
        vkBindBufferMemory( m_device, m_handle, m_memory, offset );
    }


    bool Buffer::Flush( size_t size, size_t offset )
    {
        VkMappedMemoryRange mappedRange = {};
		mappedRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size   = size;
		return vkFlushMappedMemoryRanges( m_device, 1, &mappedRange ) == VK_SUCCESS;
    }


    char* Buffer::MappedPtr() const
    {
        return static_cast< char* >( m_mappedPtr );
    }


    size_t Buffer::GetLength() const
    {
        return m_length;
    }


    BufferType Buffer::GetType() const
    {
        return m_type;
    }


    MemoryType Buffer::GetMemoryType() const
    {
        return m_memoryType;
    }


    VkBuffer Buffer::GetHandle() const
    {
        return m_handle;
    }


    VkDeviceMemory Buffer::GetMemoryHandle() const
    {
        return m_memory;
    }


    Buffer::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


} // namespace Gfx
} // namespace PG