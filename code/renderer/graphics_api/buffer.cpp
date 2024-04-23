#include "renderer/graphics_api/buffer.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/core_defines.hpp"
#include <string.h>

namespace PG::Gfx
{

uint32_t NumBytesPerElement( BufferFormat format )
{
    uint8_t sizes[] =
    {
        0,  // INVALID = 0,
        1,  // UCHAR  = 1,
        2,  // UCHAR2 = 2,
        3,  // UCHAR3 = 3,
        4,  // UCHAR4 = 4,
        1,  // CHAR  = 5,
        2,  // CHAR2 = 6,
        3,  // CHAR3 = 7,
        4,  // CHAR4 = 8,
        1,  // UCHAR_NORM  = 9,
        2,  // UCHAR2_NORM = 10,
        3,  // UCHAR3_NORM = 11,
        4,  // UCHAR4_NORM = 12,
        1,  // CHAR_NORM  = 13,
        2,  // CHAR2_NORM = 14,
        3,  // CHAR3_NORM = 15,
        4,  // CHAR4_NORM = 16,
        2,  // USHORT  = 17,
        4,  // USHORT2 = 18,
        6,  // USHORT3 = 19,
        8,  // USHORT4 = 20,
        2,  // SHORT  = 21,
        4,  // SHORT2 = 22,
        6,  // SHORT3 = 23,
        8,  // SHORT4 = 24,
        2,  // USHORT_NORM  = 25,
        4,  // USHORT2_NORM = 26,
        6,  // USHORT3_NORM = 27,
        8,  // USHORT4_NORM = 28,
        2,  // SHORT_NORM  = 29,
        4,  // SHORT2_NORM = 30,
        6,  // SHORT3_NORM = 31,
        8,  // SHORT4_NORM = 32,
        2,  // HALF  = 33,
        4,  // HALF2 = 34,
        6,  // HALF3 = 35,
        8,  // HALF4 = 36,
        4,  // FLOAT  = 37,
        8,  // FLOAT2 = 38,
        12, // FLOAT3 = 39,
        16, // FLOAT4 = 40,
        4,  // UINT  = 41,
        8,  // UINT2 = 42,
        12, // UINT3 = 43,
        16, // UINT4 = 44,
        4,  // INT  = 45,
        8,  // INT2 = 46,
        12, // INT3 = 47,
        16, // INT4 = 48,
    };

    static_assert( ARRAY_COUNT( sizes ) == Underlying( BufferFormat::NUM_BUFFER_FORMATS ) );
    return sizes[Underlying( format )];
}

int SizeOfIndexType( IndexType type )
{
    int8_t sizes[] =
    {
        2, // UNSIGNED_SHORT
        4, // UNSIGNED_INT
    };

    static_assert( ARRAY_COUNT( sizes ) == Underlying( IndexType::NUM_INDEX_TYPE ) );

    return sizes[Underlying( type )];
}

void Buffer::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vkDestroyBuffer( m_device, m_handle, nullptr );
    vkFreeMemory( m_device, m_memory, nullptr );
    m_handle = VK_NULL_HANDLE;
}

void Buffer::Map() const { VK_CHECK( vkMapMemory( m_device, m_memory, 0, VK_WHOLE_SIZE, 0, &m_mappedPtr ) ); }

void Buffer::UnMap() const
{
    vkUnmapMemory( m_device, m_memory );
    m_mappedPtr = nullptr;
}

void Buffer::BindMemory( size_t offset ) const { VK_CHECK( vkBindBufferMemory( m_device, m_handle, m_memory, offset ) ); }

void Buffer::FlushCpuWrites( size_t size, size_t offset ) const
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = m_memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    if ( !m_mappedPtr )
    {
        Map();
    }
    VK_CHECK( vkFlushMappedMemoryRanges( m_device, 1, &mappedRange ) );
}

void Buffer::FlushGpuWrites( size_t size, size_t offset ) const
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory              = m_memory;
    mappedRange.offset              = offset;
    mappedRange.size                = size;
    if ( !m_mappedPtr )
    {
        Map();
    }
    VK_CHECK( vkInvalidateMappedMemoryRanges( m_device, 1, &mappedRange ) );
}

// TODO: barriers here or externally to ensure gpu buffer commands have been completed?
void Buffer::ReadToCpu( void* dst, size_t size, size_t offset ) const
{
    if ( m_memoryType & MEMORY_TYPE_DEVICE_LOCAL )
    {
        PG_ASSERT( false, "implement me" );
        // Read back to host visible buffer first, then copy to host
        // Device& device = rg.device;
        //
        // Buffer hostVisibleBuffer = device.NewBuffer( size, BUFFER_TYPE_TRANSFER_DST, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT
        // );
        //
        // CommandBuffer cmdBuf = rg.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time copy buffer -> buffer" );
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
        if ( ( m_memoryType & MEMORY_TYPE_HOST_COHERENT ) == 0 )
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

} // namespace PG::Gfx
