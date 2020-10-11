#include "renderer/graphics_api/buffer.hpp"
#include "core/assert.hpp"
#include "core/core_defines.hpp"
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


    void Buffer::ReadToCpu( void* dst, size_t size, size_t offset ) const
    {
        bool isMappedAlready = m_mappedPtr != nullptr;
        Map();
        if ( m_memoryType & MEMORY_TYPE_HOST_VISIBLE )
        {
            // cached memory is always coherent
            bool hostVisible = (m_memoryType & MEMORY_TYPE_HOST_COHERENT) || (m_memoryType & MEMORY_TYPE_HOST_CACHED);
            if ( !hostVisible )
            {
                FlushGpuWrites( size, offset );
            }
        }
        else
        {
            PG_ASSERT( false, "implement me" );
            // Read back to host visible buffer first, then copy to host
            /*
			VkBufferCopy copyRegion = {};
			copyRegion.size = bufferSize;
			vkCmdCopyBuffer(commandBuffer, deviceBuffer, hostBuffer, 1, &copyRegion);

			// Barrier to ensure that buffer copy is finished before host reading from it
			bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
			bufferBarrier.buffer = hostBuffer;
			bufferBarrier.size = VK_WHOLE_SIZE;
			bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				1, &bufferBarrier,
				0, nullptr);

			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

			// Submit compute work
			vkResetFences(device, 1, &fence);
			const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
			VkSubmitInfo computeSubmitInfo = vks::initializers::submitInfo();
			computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
			computeSubmitInfo.commandBufferCount = 1;
			computeSubmitInfo.pCommandBuffers = &commandBuffer;
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &computeSubmitInfo, fence));
			VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

			// Make device writes visible to the host
			void *mapped;
			vkMapMemory(device, hostMemory, 0, VK_WHOLE_SIZE, 0, &mapped);
			VkMappedMemoryRange mappedRange = vks::initializers::mappedMemoryRange();
			mappedRange.memory = hostMemory;
			mappedRange.offset = 0;
			mappedRange.size = VK_WHOLE_SIZE;
			vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);

			// Copy to output
			memcpy(computeOutput.data(), mapped, bufferSize);
			vkUnmapMemory(device, hostMemory);
            */
        }
        memcpy( dst, m_mappedPtr, size == VK_WHOLE_SIZE ? m_length : size );
        if ( !isMappedAlready )
        {
            UnMap();
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