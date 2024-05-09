#pragma once

#include "renderer/graphics_api/buffer.hpp"

namespace PG::Gfx
{

enum class AccelerationStructureType : uint8_t
{
    TLAS = 0,
    BLAS = 1,

    COUNT
};

class AccelerationStructure
{
    friend class Device;

public:
    void Free();

    operator bool() const { return m_handle != VK_NULL_HANDLE; }
    const Buffer& GetBuffer() const { return m_buffer; }
    AccelerationStructureType GetType() const { return m_type; }
    VkAccelerationStructureKHR GetHandle() const { return m_handle; }
    VkDeviceAddress GetDeviceAddress() const { return m_deviceAddress; }

protected:
    Buffer m_buffer;
    AccelerationStructureType m_type;
    VkAccelerationStructureKHR m_handle = VK_NULL_HANDLE;
    VkDeviceAddress m_deviceAddress;
};

} // namespace PG::Gfx
