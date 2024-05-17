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
    VERTEX                  = 0,
    TESSELLATION_CONTROL    = 1,
    TESSELLATION_EVALUATION = 2,
    GEOMETRY                = 3,
    FRAGMENT                = 4,
    COMPUTE                 = 5,
    TASK                    = 6,
    MESH                    = 7,

    NUM_SHADER_STAGES
};

struct ShaderCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
    ShaderStage shaderStage = ShaderStage::NUM_SHADER_STAGES;
    std::vector<std::pair<std::string, std::string>> defines;
    bool generateDebugInfo = false;
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

    ShaderReflectData reflectionData;
#if USING( GPU_DATA )
    VkShaderModule handle = VK_NULL_HANDLE;
#endif // #if USING( GPU_DATA )
    ShaderStage shaderStage;

#if USING( CONVERTER )
    std::vector<uint32_t> savedSpirv;
#endif // #if USING( CONVERTER )
};

} // namespace PG
