#include "renderer/graphics_api/buffer.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/core_defines.hpp"
#include <string.h>

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


    void Buffer::Map() const
    {
        VK_CHECK_RESULT( vkMapMemory( m_device, m_memory, 0, VK_WHOLE_SIZE, 0, &m_mappedPtr ) );
    }


    void Buffer::UnMap() const
    {
        vkUnmapMemory( m_device, m_memory );
        m_mappedPtr = nullptr;
    }


    void Buffer::BindMemory( size_t offset ) const
    {
        VK_CHECK_RESULT( vkBindBufferMemory( m_device, m_handle, m_memory, offset ) );
    }


    void Buffer::FlushCpuWrites( size_t size, size_t offset ) const
    {
        VkMappedMemoryRange mappedRange = {};
		mappedRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size   = size;
        if ( !m_mappedPtr )
        {
            Map();
        }
        VK_CHECK_RESULT( vkFlushMappedMemoryRanges( m_device, 1, &mappedRange ) );
    }


    void Buffer::FlushGpuWrites( size_t size, size_t offset ) const
    {
        VkMappedMemoryRange mappedRange = {};
		mappedRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = m_memory;
		mappedRange.offset = offset;
		mappedRange.size   = size;
        if ( !m_mappedPtr )
        {
            Map();
        }
		VK_CHECK_RESULT( vkInvalidateMappedMemoryRanges( m_device, 1, &mappedRange ) );
    }


    // TODO: barriers here or externally to ensure gpu buffer commands have been completed? 
    void Buffer::ReadToCpu( void* dst, size_t size, size_t offset ) const
    {
        if ( m_memoryType & MEMORY_TYPE_DEVICE_LOCAL )
        {
            PG_ASSERT( false, "implement me" );
            // Read back to host visible buffer first, then copy to host
            // Device& device = r_globals.device;
            // 
            // Buffer hostVisibleBuffer = device.NewBuffer( size, BUFFER_TYPE_TRANSFER_DST, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            // 
            // CommandBuffer cmdBuf = r_globals.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time copy buffer -> buffer" );
            // cmdBuf.CopyBuffer( hostVisibleBuffer, *this, size, 0, offset );
            // 
			// // Barrier to ensure that buffer copy is finished before host reading from it
            // VkBufferMemoryBarrier bufferBarrier = {};
			// bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			// bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			// bufferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
			// bufferBarrier.buffer = hostVisibleBuffer.GetHandle();
			// bufferBarrier.size = VK_WHOLE_SIZE;
			// bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			// bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            // 
            // cmdBuf.PipelineBufferBarrier( VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, bufferBarrier );
            // cmdBuf.EndRecording();
            // device.Submit();
            // 
            // hostVisibleBuffer.Map();
            // memcpy( dst, hostVisibleBuffer.MappedPtr(), size == VK_WHOLE_SIZE ? m_length : size );
            // hostVisibleBuffer.Free();
        }
        else
        {
            bool isMappedAlready = m_mappedPtr != nullptr;
            Map();
            if ( (m_memoryType & MEMORY_TYPE_HOST_COHERENT) == 0 )
            {
                FlushGpuWrites( size, offset );
            }
            memcpy( dst, m_mappedPtr, size == VK_WHOLE_SIZE ? m_length : size );
            if ( !isMappedAlready )
            {
                UnMap();
            }
        }
    }


    Buffer::operator bool() const { return m_handle != VK_NULL_HANDLE; }
    char* Buffer::MappedPtr() const { return static_cast< char* >( m_mappedPtr ); }
    size_t Buffer::GetLength() const { return m_length; }
    BufferType Buffer::GetType() const { return m_type; }
    MemoryType Buffer::GetMemoryType() const { return m_memoryType; }
    VkBuffer Buffer::GetHandle() const { return m_handle; }
    VkDeviceMemory Buffer::GetMemoryHandle() const { return m_memory; }

} // namespace Gfx
} // namespace PG