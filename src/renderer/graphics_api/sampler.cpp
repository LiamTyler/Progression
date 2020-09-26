#include "renderer/graphics_api/sampler.hpp"
#include "core/assert.hpp"

namespace PG
{
namespace Gfx
{

    void Sampler::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroySampler( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }


    Sampler::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


    std::string Sampler::GetName() const { return m_desc.name; }
    FilterMode Sampler::GetMinFilter() const { return m_desc.minFilter; }
    FilterMode Sampler::GetMagFilter() const { return m_desc.magFilter; }
    WrapMode Sampler::GetWrapModeU() const { return m_desc.wrapModeU; }
    WrapMode Sampler::GetWrapModeV() const { return m_desc.wrapModeV; }
    WrapMode Sampler::GetWrapModeW() const { return m_desc.wrapModeW; }
    float Sampler::GetMaxAnisotropy() const { return m_desc.maxAnisotropy; }
    BorderColor Sampler::GetBorderColor() const { return m_desc.borderColor; }
    VkSampler Sampler::GetHandle() const { return m_handle; }

} // namespace Gfx
} // namespace PG
