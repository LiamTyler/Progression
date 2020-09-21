#pragma once

#include "asset/types/base_asset.hpp"
#include "core/platform_defines.hpp"
#include <vector>
#include <vulkan/vulkan.h>

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
};

struct Shader : public Asset
{
    void Free();

    ShaderStage shaderStage;
    VkShaderModule handle = VK_NULL_HANDLE;

#if USING( COMPILING_CONVERTER )
    std::vector< uint32_t > spirv;
#endif // #if USING( COMPILING_CONVERTER )
};

bool Shader_Load( Shader* shader, const ShaderCreateInfo& createInfo );

bool Fastfile_Shader_Load( Shader* shader, Serializer* serializer );

bool Fastfile_Shader_Save( const Shader * const shader, Serializer* serializer );

// preprocesses the shader to get a list of all the files that were #include'd
// returns false if preprocessing step failed.
bool Shader_GetIncludes( const std::string& filename, ShaderStage shaderStage, std::vector< std::string >& includedFiles );

} // namespace PG