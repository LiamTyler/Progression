#pragma once

#include "asset/types/shader.hpp"
#include <set>

namespace PG
{

struct ShaderPreprocessOutput
{
    bool success;
    std::string outputShader;
    std::set<std::string> includedFilesAbsPath;
};

ShaderPreprocessOutput PreprocessShader( const ShaderCreateInfo& createInfo, bool savePreproc );

// returns the shaderc type casted to an i32 to avoid including shaderc in the header
i32 PGShaderStageToShaderc( const ShaderStage stage );

} // namespace PG
