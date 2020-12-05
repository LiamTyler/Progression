#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "shaderc/shaderc.hpp"
#include "utils/filesystem.hpp"
#include "utils/hash.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <regex>
#include <sstream>


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


std::string CleanUpPreproc( const std::string& preprocOutput )
{
    std::stringstream ss;
    ss << preprocOutput;
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


static const char* PGShaderStageToDefine( PG::ShaderStage stage )
{
    static const char* shaderStageToDefine[] =
    {
        "IS_VERTEX_SHADER",
        "IS_TESS_CONTROL_SHADER",
        "IS_TESS_EVAL_SHADER",
        "IS_GEOMETRY_SHADER",
        "IS_FRAGMENT_SHADER",
        "IS_COMPUTE_SHADER",
    };
    
    int index = static_cast< int >( stage );
    PG_ASSERT( 0 <= index && index <= static_cast< int >( PG::ShaderStage::NUM_SHADER_STAGES ), "index '" + std::to_string( index ) + "' not in range" );
    return shaderStageToDefine[index];
}


static shaderc_shader_kind PGToShadercShaderStage( PG::ShaderStage stage )
{
    static shaderc_shader_kind convert[] =
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
    return convert[index];
}


namespace PG
{

class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    ShaderIncluder( std::vector< std::string >* includes ) : includedFiles( includes )
    {
        searchDirs[0] = PG_ASSET_DIR "shaders/";
    }

    virtual ~ShaderIncluder() = default;

    virtual shaderc_include_result* GetInclude( const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth )
    {
        shaderc_include_result* result = new shaderc_include_result;
        memset( result, 0, sizeof( shaderc_include_result ) );

        searchDirs[1] = GetParentPath( requesting_source );
        for ( int i = 0; i < 2; ++i )
        {
            const std::string& dir = searchDirs[i];
            std::string filename = dir + requested_source;
            if ( PathExists( filename ) )
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

    std::string searchDirs[2];
    std::vector< std::string >* includedFiles;
};


ShaderPreprocessOutput PreprocessShader( const ShaderCreateInfo& createInfo )
{
    ShaderPreprocessOutput output;
    output.success = false;
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0 );
    options.SetSourceLanguage( shaderc_source_language_glsl );
    auto includer = std::make_unique< ShaderIncluder >( &output.includedFiles );
    options.SetIncluder( std::move( includer ) );
    
    std::string shaderStageDefine = PGShaderStageToDefine( createInfo.shaderStage );
    options.AddMacroDefinition( "PG_SHADER_CODE", "1" );
    options.AddMacroDefinition( shaderStageDefine, "1" );
    for ( const auto& [symbol, value] : createInfo.defines )
    {
        options.AddMacroDefinition( symbol, value );
    }

    size_t fileLen;
    char* fileContents = ReadShaderFile( createInfo.filename, fileLen );
    shaderc_shader_kind shadercStage = PGToShadercShaderStage( createInfo.shaderStage );
    shaderc::PreprocessedSourceCompilationResult result = compiler.PreprocessGlsl( fileContents, shadercStage, createInfo.filename.c_str(), options );

    if ( result.GetCompilationStatus() != shaderc_compilation_status_success )
    {
        LOG_ERR( "Preprocess GLSL failed:\n%s\n", result.GetErrorMessage().c_str() );
        return output;
    }

    output.success = true;
    output.outputShader = { result.cbegin(), result.cend() };
       
    if ( createInfo.writePreproc )
    {
        //output.outputShader = CleanUpPreproc( output.outputShader );
        size_t seed = 0;
        hash_combine( seed, std::string( "PG_SHADER_CODE1" ) );
        hash_combine( seed, shaderStageDefine + "1" );
        for ( const auto& [define, value] : createInfo.defines )
        {
            hash_combine( seed, define + value );
        }
        std::string preprocFilename = PG_ASSET_DIR "cache/shader_preproc/" + GetRelativeFilename( createInfo.filename ) + "_" + std::to_string( seed ) + ".glsl";
        std::ofstream out( preprocFilename );
        if ( !out )
        {
            LOG_ERR( "Could not output preproc file '%s'\n",  preprocFilename.c_str() );
            output.success = false;
            return output;
        }
        out << output.outputShader << std::endl;
        
        // write list of #defines at the bottom for easier debugging
        out << "// #define PG_SHADER_CODE 1\n";
        out << "// #define " << shaderStageDefine << " 1\n";
        for ( const auto& [define, value] : createInfo.defines )
        {
            out << "// #define " << define << " " << value << "\n";
        }
    }

    return output;
}


int PGShaderStageToShaderc( const ShaderStage stage )
{
    return static_cast< int >( PGToShadercShaderStage( stage ) );
}

} // namespace PG
