#include "renderer/graphics_api/buffer.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/core_defines.hpp"
#include <string.h>

namespace PG::Gfx
{

uint32_t NumBytesPerElement( BufferFormat format )
{
    uint8_t sizes[] = {
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
    int8_t sizes[] = {
        2, // UNSIGNED_SHORT
        4, // UNSIGNED_INT
    };

    static_assert( ARRAY_COUNT( sizes ) == Underlying( IndexType::NUM_INDEX_TYPE ) );

    return sizes[Underlying( type )];
}

void Buffer::Free()
{
    PG_ASSERT( m_handle != VK_NULL_HANDLE );
    vmaDestroyBuffer( rg.device.GetAllocator(), m_handle, m_allocation );
#if USING( DEBUG_BUILD )
    m_handle = VK_NULL_HANDLE;
    if ( debugName )
    {
        free( debugName );
        debugName = nullptr;
    }
#endif // #if USING( DEBUG_BUILD )
}

char* Buffer::Map()
{
    if ( !m_persistent )
        vmaMapMemory( rg.device.GetAllocator(), m_allocation, &m_mappedPtr );
    return GetMappedPtr();
}

void Buffer::UnMap()
{
    if ( !m_persistent )
    {
        vmaUnmapMemory( rg.device.GetAllocator(), m_allocation );
        m_mappedPtr = nullptr;
    }
}

void Buffer::FlushCpuWrites( size_t size, size_t offset )
{
    if ( !m_coherent )
        vmaFlushAllocation( rg.device.GetAllocator(), m_allocation, offset, size );
}

void Buffer::FlushGpuWrites( size_t size, size_t offset )
{
    if ( !m_coherent )
        vmaInvalidateAllocation( rg.device.GetAllocator(), m_allocation, offset, size );
}

VkDeviceAddress Buffer::GetDeviceAddress() const
{
    VkBufferDeviceAddressInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = m_handle;
    return vkGetBufferDeviceAddress( rg.device, &info );
}

VkDescriptorType Buffer::GetDescriptorType() const
{
    if ( IsSet( m_bufferUsage, BufferUsage::STORAGE ) )
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    if ( IsSet( m_bufferUsage, BufferUsage::UNIFORM ) )
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if ( IsSet( m_bufferUsage, BufferUsage::STORAGE_TEXEL ) )
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    if ( IsSet( m_bufferUsage, BufferUsage::UNIFORM_TEXEL ) )
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;

    return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

} // namespace PG::Gfx
