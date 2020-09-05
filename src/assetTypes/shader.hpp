#pragma once

#include "assetTypes/base_asset.hpp"
#include <vector>

class Serializer;

namespace Progression
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
public:
    Shader() = default;

    ShaderStage shaderStage;
    std::vector< uint32_t > spirv;
};

bool Shader_Load( Shader* script, const ShaderCreateInfo& createInfo );

bool Fastfile_Shader_Load( Shader* script, Serializer* serializer );

bool Fastfile_Shader_Save( const Shader * const script, Serializer* serializer );

// preprocesses the shader to get a list of all the files that were #include'd
// returns false if preprocessing step failed.
bool Shader_GetIncludes( const std::string& filename, ShaderStage shaderStage, std::vector< std::string >& includedFiles );

} // namespace Progression