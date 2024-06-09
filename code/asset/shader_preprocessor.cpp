#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"
#include "shaderc/shaderc.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include <cstring>
#include <fstream>
#include <mutex>
#include <regex>
#include <sstream>

// Cache that stores shader files in memory once they are read from disk the first time.
//  Future requests to load from disk will use the cache instead
#define FILE_CACHE IN_USE

static std::shared_ptr<std::string> ReadShaderFile( const std::string& filename )
{
    std::ifstream in( filename, std::ios::binary );
    if ( !in )
    {
        return nullptr;
    }

    std::shared_ptr<std::string> strPtr = std::make_shared<std::string>();
    in.seekg( 0, std::ios::end );
    size_t fileLen = in.tellg();
    strPtr->resize( fileLen );
    in.seekg( 0 );
    in.read( strPtr->data(), fileLen );

    return strPtr;
}

std::string CleanUpPreproc( const std::string& preprocOutput )
{
    std::stringstream ss;
    ss << preprocOutput;
    std::string result;
    std::string line;
    i32 numBlankLines = 0;
    while ( std::getline( ss, line ) )
    {
        if ( line.length() == 0 )
        {
            ++numBlankLines;
        }
        else
        {
            numBlankLines = 0;
        }

        if ( numBlankLines < 2 )
        {
            line = std::regex_replace( line, std::regex( " \\. " ), "." );
            line = std::regex_replace( line, std::regex( "\\. " ), "." );
            result += line + '\n';
        }
    }

    return result;
}

static const char* PGShaderStageToDefine( PG::ShaderStage stage )
{
    static const char* shaderStageToDefine[] = {
        "IS_VERTEX_SHADER",
        "IS_TESS_CONTROL_SHADER",
        "IS_TESS_EVAL_SHADER",
        "IS_GEOMETRY_SHADER",
        "IS_FRAGMENT_SHADER",
        "IS_COMPUTE_SHADER",
        "IS_TASK_SHADER",
        "IS_MESH_SHADER",
    };

    i32 index = static_cast<i32>( stage );
    PG_ASSERT( 0 <= index && index <= static_cast<i32>( PG::ShaderStage::NUM_SHADER_STAGES ), "index %d not in range", index );
    return shaderStageToDefine[index];
}

static shaderc_shader_kind PGToShadercShaderStage( PG::ShaderStage stage )
{
    static shaderc_shader_kind convert[] = {
        shaderc_vertex_shader,   // VERTEX
        shaderc_geometry_shader, // GEOMETRY
        shaderc_fragment_shader, // FRAGMENT
        shaderc_compute_shader,  // COMPUTE
        shaderc_task_shader,     // TASK
        shaderc_mesh_shader,     // MESH
    };

    i32 index = static_cast<i32>( stage );
    PG_ASSERT( 0 <= index && index <= static_cast<i32>( PG::ShaderStage::NUM_SHADER_STAGES ), "index %d not in range", index );
    return convert[index];
}

#if USING( FILE_CACHE )
static std::unordered_map<std::string, std::shared_ptr<std::string>> s_fileCache;
static std::mutex s_fileCacheLock;

std::shared_ptr<std::string> GetFileContents( const std::string& absFilePath )
{
    std::scoped_lock lock( s_fileCacheLock );
    if ( s_fileCache.contains( absFilePath ) )
        return s_fileCache[absFilePath];

    auto fileContents        = ReadShaderFile( absFilePath );
    s_fileCache[absFilePath] = fileContents;
    return fileContents;
}

#else // #if USING( FILE_CACHE )

std::shared_ptr<std::string> GetFileContents( const std::string& absFilePath ) { return ReadShaderFile( absFilePath ); }

#endif // #else // #if USING( FILE_CACHE )

namespace PG
{

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    ShaderIncluder( ShaderPreprocessOutput* outputData )
    {
        output = outputData;
        fileContentPtrs.reserve( 16 );
    }

    virtual ~ShaderIncluder() = default;

    virtual shaderc_include_result* GetInclude(
        const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth )
    {
        shaderc_include_result* result = new shaderc_include_result;
        memset( result, 0, sizeof( shaderc_include_result ) );

        std::string filename = PG_ASSET_DIR "shaders/" + std::string( requested_source );
        if ( PathExists( filename ) )
        {
            output->includedFilesAbsPath.insert( filename );
            auto fileContents = GetFileContents( filename );
            if ( fileContents )
            {
                char* name = static_cast<char*>( malloc( filename.length() + 1 ) );
                strcpy( name, filename.c_str() );
                result->source_name        = name;
                result->source_name_length = filename.length();
                result->content            = fileContents->c_str();
                result->content_length     = fileContents->size();
                fileContentPtrs.push_back( fileContents );
            }
        }

        return result;
    }

    virtual void ReleaseInclude( shaderc_include_result* data )
    {
        if ( data->source_name_length > 0 )
        {
            free( (void*)data->source_name );
        }

        delete data;
    }

    ShaderPreprocessOutput* output;
    std::vector<std::shared_ptr<std::string>> fileContentPtrs;
};

ShaderPreprocessOutput PreprocessShader( const ShaderCreateInfo& createInfo, bool savePreproc )
{
    ShaderPreprocessOutput output;
    output.success = false;

    std::shared_ptr<std::string> fileContents = GetFileContents( GetAbsPath_ShaderFilename( createInfo.filename ) );
    if ( !fileContents )
        return output;

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3 );
    options.SetSourceLanguage( shaderc_source_language_glsl );
    auto includer = std::make_unique<ShaderIncluder>( &output );
    options.SetIncluder( std::move( includer ) );

    std::string shaderStageDefine = PGShaderStageToDefine( createInfo.shaderStage );
    auto defines                  = createInfo.defines;
    defines.emplace_back( "PG_SHADER_CODE 1" );
    defines.emplace_back( shaderStageDefine + " 1" );
    for ( const std::string& define : defines )
    {
        options.AddMacroDefinition( define );
    }

    shaderc_shader_kind shadercStage = PGToShadercShaderStage( createInfo.shaderStage );
    shaderc::PreprocessedSourceCompilationResult result =
        compiler.PreprocessGlsl( fileContents->c_str(), shadercStage, createInfo.filename.c_str(), options );

    if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
    {
        LOG_ERR( "Preprocess GLSL failed: %s", result.GetErrorMessage().c_str() );
        return output;
    }

    output.success      = true;
    output.outputShader = { result.cbegin(), result.cend() };

    if ( savePreproc )
    {
        // output.outputShader         = CleanUpPreproc( output.outputShader );
        std::string preprocFilename = PG_ASSET_DIR "cache/shader_preproc/" + createInfo.name + ".glsl";
        std::ofstream out( preprocFilename );
        if ( !out )
        {
            LOG_ERR( "Could not output preproc file '%s'", preprocFilename.c_str() );
            output.success = false;
            return output;
        }
        for ( const std::string& define : defines )
        {
            out << "// #define " << define << "\n";
        }

        out << output.outputShader << std::endl;
    }

    return output;
}

i32 PGShaderStageToShaderc( const ShaderStage stage ) { return static_cast<i32>( PGToShadercShaderStage( stage ) ); }

} // namespace PG
