#pragma once

#include "asset/types/base_asset.hpp"
#include "shared/platform_defines.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/vulkan.hpp"
#include <vector>

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
    std::vector< std::pair< std::string, std::string > > defines;
    bool generateDebugInfo = false;
};

struct ShaderResourceLayout
{
	uint32_t inputMask        = 0;
	uint32_t outputMask       = 0;
    uint32_t pushConstantSize = 0;
	Gfx::DescriptorSetLayout sets[PG_MAX_NUM_DESCRIPTOR_SETS];
};

struct Shader : public BaseAsset
{
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;

    ShaderStage shaderStage;
    std::string entryPoint;
    VkShaderModule handle = VK_NULL_HANDLE;
    ShaderResourceLayout resourceLayout;

#if USING( CONVERTER )
    std::vector< uint32_t > savedSpirv;
#endif // #if USING( CONVERTER )
};

} // namespace PG