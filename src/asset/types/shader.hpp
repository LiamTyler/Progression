#pragma once

#include "asset/types/base_asset.hpp"
#include "core/platform_defines.hpp"
#include "renderer/graphics_api/descriptor.hpp"
#include "renderer/vulkan.hpp"
#include <vector>

class Serializer;

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

struct ShaderCreateInfo
{
    std::string name;
    std::string filename;
    ShaderStage shaderStage;
    std::vector< std::pair< std::string, std::string > > defines;
};

struct ShaderResourceLayout
{
	uint32_t inputMask        = 0;
	uint32_t outputMask       = 0;
    uint32_t pushConstantSize = 0;
	Gfx::DescriptorSetLayout sets[PG_MAX_NUM_DESCRIPTOR_SETS];
};

struct Shader : public Asset
{
    void Free();

    ShaderStage shaderStage;
    std::string entryPoint;
    VkShaderModule handle = VK_NULL_HANDLE;
    ShaderResourceLayout resourceLayout;

#if USING( COMPILING_CONVERTER )
    std::vector< uint32_t > spirv;
#endif // #if USING( COMPILING_CONVERTER )
};

bool Shader_Load( Shader* shader, const ShaderCreateInfo& createInfo );

bool Fastfile_Shader_Load( Shader* shader, Serializer* serializer );

bool Fastfile_Shader_Save( const Shader * const shader, Serializer* serializer );

} // namespace PG