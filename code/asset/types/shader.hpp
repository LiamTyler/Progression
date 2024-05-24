#pragma once

#include "asset/types/base_asset.hpp"
#include "core/feature_defines.hpp"
#include "shared/math_vec.hpp"
#include <vector>

#if USING( GPU_DATA )
#include "renderer/vulkan.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

enum class ShaderStage : uint8_t
{
    VERTEX   = 0,
    GEOMETRY = 1,
    FRAGMENT = 2,
    COMPUTE  = 3,
    TASK     = 4,
    MESH     = 5,

    NUM_SHADER_STAGES
};

struct ShaderCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
    ShaderStage shaderStage = ShaderStage::NUM_SHADER_STAGES;
    std::vector<std::string> defines;
};

std::string GetAbsPath_ShaderFilename( const std::string& filename );

struct ShaderReflectData
{
    uvec3 workgroupSize         = uvec3( 0 );
    uint16_t pushConstantSize   = 0;
    uint16_t pushConstantOffset = 0;
};

struct Shader : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;

    const ShaderReflectData& GetReflectionData() const { return m_reflectionData; }
    ShaderStage GetShaderStage() const { return m_shaderStage; }

#if USING( GPU_DATA )
    VkShaderModule GetHandle() const { return m_handle; }
    operator VkShaderModule() const { return m_handle; }
#endif // #if USING( GPU_DATA )

#if USING( CONVERTER )
    std::vector<uint32_t> savedSpirv;
#endif // #if USING( CONVERTER )

private:
#if USING( GPU_DATA )
    VkShaderModule m_handle = VK_NULL_HANDLE;
#endif // #if USING( GPU_DATA )
    ShaderStage m_shaderStage;
    ShaderReflectData m_reflectionData;
};

} // namespace PG
