#pragma once

#include <string>
#include <vector>

namespace PG
{

using DefineList = std::vector< std::pair< std::string, std::string > >;

struct ShaderPreprocessOutput
{
    bool success;
    std::string outputShader;
    std::vector< std::string > includedFiles;
};

ShaderPreprocessOutput PreprocessShader( const std::string& filename, const DefineList& defines );

ShaderPreprocessOutput PreprocessShaderForIncludeListOnly( const std::string& filename, const DefineList& defines );

} // namespace PG