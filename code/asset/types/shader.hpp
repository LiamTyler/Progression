#pragma once

#include "asset/types/base_asset.hpp"
#include "core/feature_defines.hpp"
#include <vector>

// ShaderCreateInfo is needed for asset parsing + tools that don't need any rendering + gpu code
#if USING( GPU_STRUCTS )
    #include "renderer/graphics_api/descriptor.hpp"
#endif // #if USING( GPU_STRUCTS )

namespace PG
{

enum class ShaderStage
{
    VERTEX                  = 0,
    TESSELLATION_CONTROL    = 1,
    TESSELLATION_EVALUATION = 2,
    GEOMETRY                = 3,
    FRAGMENT                = 4,
    COMPUTE                 = 5,

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

struct ShaderResourceLayout
{
    uint32_t inputMask        = 0;
    uint32_t outputMask       = 0;
    uint32_t pushConstantSize = 0;
#if USING( GPU_STRUCTS )
    Gfx::DescriptorSetLayout sets[PG_MAX_NUM_DESCRIPTOR_SETS];
#endif // #if USING( GPU_STRUCTS )
};

struct Shader : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;

    ShaderStage shaderStage;
    std::string entryPoint;
#if USING( GPU_DATA )
    VkShaderModule handle = VK_NULL_HANDLE;
#endif // #if USING( GPU_DATA )
    ShaderResourceLayout resourceLayout;

#if USING( CONVERTER )
    std::vector<uint32_t> savedSpirv;
#endif // #if USING( CONVERTER )
};

} // namespace PG
