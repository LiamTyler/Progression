#pragma once

#include "renderer/vulkan.hpp"

namespace PG::Gfx
{

enum class BufferFormat : u8
{
    INVALID = 0,

    UCHAR  = 1,
    UCHAR2 = 2,
    UCHAR3 = 3,
    UCHAR4 = 4,

    CHAR  = 5,
    CHAR2 = 6,
    CHAR3 = 7,
    CHAR4 = 8,

    UCHAR_NORM  = 9,
    UCHAR2_NORM = 10,
    UCHAR3_NORM = 11,
    UCHAR4_NORM = 12,

    CHAR_NORM  = 13,
    CHAR2_NORM = 14,
    CHAR3_NORM = 15,
    CHAR4_NORM = 16,

    USHORT  = 17,
    USHORT2 = 18,
    USHORT3 = 19,
    USHORT4 = 20,

    SHORT  = 21,
    SHORT2 = 22,
    SHORT3 = 23,
    SHORT4 = 24,

    USHORT_NORM  = 25,
    USHORT2_NORM = 26,
    USHORT3_NORM = 27,
    USHORT4_NORM = 28,

    SHORT_NORM  = 29,
    SHORT2_NORM = 30,
    SHORT3_NORM = 31,
    SHORT4_NORM = 32,

    HALF  = 33,
    HALF2 = 34,
    HALF3 = 35,
    HALF4 = 36,

    FLOAT  = 37,
    FLOAT2 = 38,
    FLOAT3 = 39,
    FLOAT4 = 40,

    UINT  = 41,
    UINT2 = 42,
    UINT3 = 43,
    UINT4 = 44,

    INT  = 45,
    INT2 = 46,
    INT3 = 47,
    INT4 = 48,

    NUM_BUFFER_FORMATS
};

u32 NumBytesPerElement( BufferFormat format );

enum class IndexType : u8
{
    UNSIGNED_SHORT = 0,
    UNSIGNED_INT   = 1,

    NUM_INDEX_TYPE
};

i32 SizeOfIndexType( IndexType type );

enum class BufferUsage : u32
{
    NONE          = 0,
    TRANSFER_SRC  = 1 << 0,
    TRANSFER_DST  = 1 << 1,
    UNIFORM_TEXEL = 1 << 2,
    STORAGE_TEXEL = 1 << 3,
    UNIFORM       = 1 << 4,
    STORAGE       = 1 << 5,
    INDEX         = 1 << 6,
    VERTEX        = 1 << 7,
    INDIRECT      = 1 << 8,

    // VK_KHR_buffer_device_address
    DEVICE_ADDRESS = 0x00020000,

    // Provided by VK_KHR_acceleration_structure
    AS_BUILD_INPUT_READ_ONLY = 0x00080000,
    AS_STORAGE               = 0x00100000,

    // Provided by VK_KHR_ray_tracing_pipeline
    SHADER_BINDING_TABLE = 0x00000400,

    // Provided VK_EXT_descriptor_buffer
    SAMPLER_DESCRIPTOR          = 0x00200000,
    RESOURCE_DESCRIPTOR         = 0x00400000,
    PUSH_DESCRIPTORS_DESCRIPTOR = 0x04000000,
};
PG_DEFINE_ENUM_OPS( BufferUsage );

struct BufferCreateInfo
{
    size_t size; // in bytes
    void* initalData               = nullptr;
    BufferUsage bufferUsage        = BufferUsage::TRANSFER_SRC | BufferUsage::TRANSFER_DST | BufferUsage::DEVICE_ADDRESS;
    VmaMemoryUsage memoryUsage     = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags flags = 0;
};

class Buffer
{
    friend class Device;
    friend class TaskGraph;

public:
    void Free();
    char* Map();
    void UnMap();
    void FlushCpuWrites( size_t size = VK_WHOLE_SIZE, size_t offset = 0 );
    void FlushGpuWrites( size_t size = VK_WHOLE_SIZE, size_t offset = 0 );
    VkDeviceAddress GetDeviceAddress() const;
    VkDescriptorType GetDescriptorType() const;

    operator bool() const { return m_handle != VK_NULL_HANDLE; }
    operator VkBuffer() const { return m_handle; }
    size_t GetSize() const { return m_size; }
    VkBuffer GetHandle() const { return m_handle; }
    VmaAllocation GetAllocation() const { return m_allocation; }
    char* GetMappedPtr() const { return static_cast<char*>( m_mappedPtr ); }
    u16 GetBindlessIndex() const { return m_bindlessIndex; }
    DEBUG_BUILD_ONLY( const char* GetDebugName() const { return debugName; } );

private:
    DEBUG_BUILD_ONLY( char* debugName = nullptr );
    size_t m_size; // in bytes
    VkBuffer m_handle;
    VmaAllocation m_allocation;
    void* m_mappedPtr = nullptr;
    BufferUsage m_bufferUsage;
    VmaMemoryUsage m_memoryUsage;
    u16 m_bindlessIndex;
    bool m_persistent = false;
    bool m_coherent   = false;
};

} // namespace PG::Gfx
