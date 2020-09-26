#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "shaderc/shaderc.hpp"
#include <fstream>
#include <sstream>
#include <regex>

static std::string s_searchDirs[] =
{
    PG_ASSET_DIR "shaders/"
};


static char* ReadShaderFile( const std::string& filename, size_t& fileLen )
{
    std::ifstream in( filename, std::ios::binary );
    if ( !in )
    {
        return nullptr;
    }

    in.seekg( 0, std::ios::end );
    fileLen = in.tellg();
    char* buffer = static_cast< char* >( malloc( fileLen + 1 ) );
    in.seekg( 0 );
    in.read( buffer, fileLen );
    buffer[fileLen] = '\0';

    return buffer;
}


std::string CleanShaderPreproc( const std::string& preproc )
{
    std::stringstream ss;
    ss << preproc;
    std::string result;
    std::string line;
    int numBlankLines = 0;
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
            result += line + '\n';
        }
    }

    return result;
}


class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    virtual ~ShaderIncluder() = default;

    virtual shaderc_include_result* GetInclude( const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth )
    {
        shaderc_include_result* result = new shaderc_include_result;
        result->source_name        = nullptr;
        result->source_name_length = 0;
        result->content_length     = 0;
        result->content            = 0;

        for ( const auto& dir : s_searchDirs )
        {
            std::string filename = dir + requested_source;
            if ( FileExists( filename ) )
            {
                size_t fileLen;
                char* fileContents = ReadShaderFile( filename, fileLen );
                if ( fileContents )
                {
                    char* name = static_cast< char* >( malloc( filename.length() + 1 ) );
                    strcpy( name, filename.c_str() );
                    result->source_name        = name;
                    result->source_name_length = filename.length();
                    result->content            = fileContents;
                    result->content_length     = fileLen;
                    includedFiles->push_back( filename );
                }
            }
        }

        return result;
    }

    virtual void ReleaseInclude( shaderc_include_result* data )
    {
        if ( data->source_name_length > 0 )
        {
            free( (void*)data->source_name );
            free( (void*)data->content );
        }
        delete data;
    }


    std::vector< std::string >* includedFiles;
};

static shaderc_shader_kind PGToShadercShaderStage( PG::ShaderStage stage )
{
    shaderc_shader_kind convert[] =
    {
        shaderc_vertex_shader,          // VERTEX
        shaderc_tess_control_shader,    // TESSELLATION_CONTROL
        shaderc_tess_evaluation_shader, // TESSELLATION_EVALUATION
        shaderc_geometry_shader,        // GEOMETRY
        shaderc_fragment_shader,        // FRAGMENT
        shaderc_compute_shader,         // COMPUTE
    };
    
    int index = static_cast< int >( stage );
    PG_ASSERT( 0 <= index && index <= static_cast< int >( PG::ShaderStage::NUM_SHADER_STAGES ), "index '" + std::to_string( index ) + "' not in range" );
    return convert[static_cast< int >( stage )];
}


static std::string PreprocessShader( const std::string& source_name, shaderc_shader_kind kind, char* shaderFileContent, std::vector< std::string >& includedFiles )
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    auto includer = std::make_unique< ShaderIncluder >();
    includer->includedFiles = &includedFiles;
    options.SetIncluder( std::move( includer ) );

    options.AddMacroDefinition( "PG_SHADER_CODE", "1" );
    // shaderc_glsl_infer_from_source
    shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl( shaderFileContent, kind, source_name.c_str(), options );

    if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
    {
        LOG_ERR( "Preprocess GLSL failed:\n%s\n", result.GetErrorMessage().c_str() );
        return "";
    }

    return { result.cbegin(), result.cend() };
}


static std::vector< uint32_t > CompilePreprocessedShaderToSPIRV( const std::string& source_name, shaderc_shader_kind kind, const std::string& source )
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetOptimizationLevel( shaderc_optimization_level_zero );

    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( source, kind, source_name.c_str(), options );

    if ( module.GetCompilationStatus() != shaderc_compilation_status_success )
    {
        std::string cleaned = CleanShaderPreproc( source );
        LOG_ERR( "Compiling shader '%s' failed: '%s'\n", source_name.c_str(), module.GetErrorMessage().c_str() );
        LOG( "Preprocessed shader for reference:\n---------------------------------------------\n" );
        LOG( "%s\n---------------------------------------------\n", cleaned.c_str() );
        return std::vector< uint32_t >();
    }

    return { module.cbegin(), module.cend() };
}


static VkShaderModule CreateShaderModule( const uint32_t* spirv, size_t sizeInBytes )
{
    VkShaderModule module;
    VkShaderModuleCreateInfo vkShaderInfo = {};
    vkShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderInfo.codeSize = sizeInBytes;
    vkShaderInfo.pCode    = spirv;
    VkResult ret = vkCreateShaderModule( PG::Gfx::r_globals.device.GetHandle(), &vkShaderInfo, nullptr, &module );
    if ( ret != VK_SUCCESS )
    {
        LOG_ERR( "Could not create shader module from spirv\n" );
        return VK_NULL_HANDLE;
    }

    return module;
}


namespace PG
{


void Shader::Free()
{
#if !USING( COMPILING_CONVERTER )
    vkDestroyShaderModule( PG::Gfx::r_globals.device.GetHandle(), handle, nullptr );
#endif // #if !USING( COMPILING_CONVERTER )
}


bool Shader_Load( Shader* shader, const ShaderCreateInfo& createInfo )
{
    PG_ASSERT( shader );
    shader->name = createInfo.name;
    shader->shaderStage = createInfo.shaderStage;

    size_t fileLen;
    char* fileContent = ReadShaderFile( createInfo.filename, fileLen );
    if ( !fileContent )
    {
        LOG_ERR( "Could not open shader file '%s' for shader '%s'\n", createInfo.filename.c_str(), createInfo.name.c_str() );
        return false;
    }

    shaderc_shader_kind shaderStage = PGToShadercShaderStage( createInfo.shaderStage );
    std::vector< std::string > includedFiles;
    std::string preproc = PreprocessShader( createInfo.name, shaderStage, fileContent, includedFiles );
    free( fileContent );
    if ( preproc.empty() )
    {
        return false;
    }
    
    std::vector< uint32_t > spirv;
    spirv = CompilePreprocessedShaderToSPIRV( createInfo.name, shaderStage, preproc );
    
    if ( spirv.empty() )
    {
        return false;
    }

#if USING( COMPILING_CONVERTER )
    shader->spirv = std::move( spirv );
#else // #if USING( COMPILING_CONVERTER )
    shader->handle = CreateShaderModule( spirv.data(), 4 * spirv.size() );
    if ( shader->handle == VK_NULL_HANDLE )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SHADER_NAME( shader, shader->name );
#endif // #else // #if USING( COMPILING_CONVERTER )

    return true;
}


bool Fastfile_Shader_Load( Shader* shader, Serializer* serializer )
{
    PG_ASSERT( shader && serializer );
    serializer->Read( shader->name );
    serializer->Read( shader->shaderStage );
    std::vector< uint32_t > spirv;
    serializer->Read( spirv );

#if !USING( COMPILING_CONVERTER )
    shader->handle = CreateShaderModule( spirv.data(), 4 * spirv.size() );
    if ( shader->handle == VK_NULL_HANDLE )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SHADER_NAME( shader, shader->name );
#endif // #if !USING( COMPILING_CONVERTER )

    return true;
}


bool Fastfile_Shader_Save( const Shader * const shader, Serializer* serializer )
{
    PG_ASSERT( shader && serializer );
#if !USING( COMPILING_CONVERTER )
    PG_ASSERT( false, "Spirv code is only kept around in Converter builds for saving" );
#else // #if !USING( COMPILING_CONVERTER )
    PG_ASSERT( shader && serializer );
    serializer->Write( shader->name );
    serializer->Write( shader->shaderStage );

    serializer->Write( shader->spirv );
#endif // #else // #if !USING( COMPILING_CONVERTER )

    return true;
}


bool Shader_GetIncludes( const std::string& filename, ShaderStage shaderStage, std::vector< std::string >& includedFiles )
{
    size_t fileLen;
    char* fileContent = ReadShaderFile( filename, fileLen );
    if ( !fileContent )
    {
        LOG_ERR( "Could not open shader file '%s'\n", filename );
        return false;
    }

    std::string preproc = PreprocessShader( "shader", PGToShadercShaderStage( shaderStage ), fileContent, includedFiles );
    if ( preproc.empty() )
    {
        return false;
    }

    return true;
}


} // namespace PG
