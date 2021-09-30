#pragma once

#include "asset/types/shader.hpp"

namespace PG
{

struct ShaderPreprocessOutput
{
    bool success;
    std::string outputShader;
    std::vector< std::string > includedFiles;
};

ShaderPreprocessOutput PreprocessShader( const ShaderCreateInfo& createInfo, bool savePreproc );

// returns the shaderc type casted to an int to avoid including shaderc in the header
int PGShaderStageToShaderc( const ShaderStage stage );

} // namespace PG