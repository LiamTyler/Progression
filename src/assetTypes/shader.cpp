#include "assetTypes/shader.hpp"
#include "assert.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "shaderc/libshaderc/include/shaderc/shaderc.hpp"
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

static shaderc_shader_kind PGToShadercShaderStage( Progression::ShaderStage stage )
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
    PG_ASSERT( 0 <= index && index <= static_cast< int >( Progression::ShaderStage::NUM_SHADER_STAGES ), "index '" + std::to_string( index ) + "' not in range" );
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


namespace Progression
{


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
    std::string preproc = PreprocessShader( createInfo.name, shaderStage, fileContent,includedFiles );
    if ( preproc.empty() )
    {
        return false;
    }

    shader->spirv = CompilePreprocessedShaderToSPIRV( createInfo.name, shaderStage, preproc );
    
    return !shader->spirv.empty();
}


bool Fastfile_Shader_Load( Shader* shader, Serializer* serializer )
{
    PG_ASSERT( shader && serializer );
    serializer->Read( shader->name );
    serializer->Read( shader->shaderStage );
    serializer->Read( shader->spirv );

    return true;
}


bool Fastfile_Shader_Save( const Shader * const shader, Serializer* serializer )
{
    PG_ASSERT( shader && serializer );
    serializer->Write( shader->name );
    serializer->Write( shader->shaderStage );
    serializer->Write( shader->spirv );

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


} // namespace Progression
