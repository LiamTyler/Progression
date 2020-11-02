#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "core/feature_defines.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "shaderc/shaderc.hpp"
#include "spirv_cross/spirv_cross.hpp"
#include <fstream>
#include <sstream>

namespace PG
{


class ShaderPreprocessor
{
public:
    ShaderPreprocessor() = default;

    bool Preprocess( const std::string& pathToShader )
    {
        includeSearchDirs =
        {
            PG_ASSET_DIR "shaders/",
            GetParentPath( pathToShader ),
        };
        
        return PreprocessInternal( pathToShader );
    }

    bool ParseForIncludesOnly( const std::string& pathToShader )
    {
        includeSearchDirs =
        {
            PG_ASSET_DIR "shaders/",
            GetParentPath( pathToShader ),
        };

        return PreprocessForIncludesOnlyInternal( pathToShader );
    }

    std::vector< std::string > includeSearchDirs;
    std::string outputShader;
    std::vector< std::string > includedFiles;

private:

    bool GetIncludePath( const std::string& shaderIncludeLine, std::string& absolutePath ) const
    {
        size_t includeStartPos = 10;
        size_t includeEndPos = shaderIncludeLine.find( '"', includeStartPos );
        if ( includeEndPos == std::string::npos )
        {
            LOG_ERR( "Invalid include on line '%s'\n", shaderIncludeLine.c_str() );
            return false;
        }
        std::string relativeInclude = shaderIncludeLine.substr( includeStartPos, includeEndPos - includeStartPos );

        std::string absoluteIncludePath;
        for ( const auto& dir : includeSearchDirs )
        {
            std::string filename = dir + relativeInclude;
            if ( FileExists( filename ) )
            {
                absolutePath = filename;
                return true;
            }
        }
        LOG_ERR( "Could not find include '%s'\n", shaderIncludeLine.c_str() );

        return false;
    }

    bool PreprocessInternal( const std::string& path )
    {
        std::ifstream inFile( path );
        if ( !inFile )
        {
            LOG_ERR( "Could not open shader file '%s'\n", path.c_str() );
            return false;
        }

        uint32_t lineNumber = 0;
        std::string line;
        while ( std::getline( inFile, line ) )
        {
            if ( line.find( "#include \"" ) == 0 )
            {
                std::string absoluteIncludePath;
                if ( !GetIncludePath( line, absoluteIncludePath ) )
                {
                    return false;
                }
                includedFiles.push_back( absoluteIncludePath );

                outputShader += "#line 1 \"" + absoluteIncludePath + "\"\n";
                if ( !PreprocessInternal( absoluteIncludePath ) )
                {
                    return false;
                }
                outputShader += "#line " + std::to_string( lineNumber + 2 ) + " \"" + path + "\"\n";
            }
            else
            {
                outputShader += line + '\n';
            }

            ++lineNumber;
        }

        return true;
    }

    bool PreprocessForIncludesOnlyInternal( const std::string& path )
    {
        std::ifstream inFile( path );
        if ( !inFile )
        {
            LOG_ERR( "Could not open shader file '%s'\n", path.c_str() );
            return false;
        }

        std::string line;
        while ( std::getline( inFile, line ) )
        {
            if ( line.find( "#include \"" ) == 0 )
            {
                std::string absoluteIncludePath;
                if ( !GetIncludePath( line, absoluteIncludePath ) )
                {
                    return false;
                }
                includedFiles.push_back( absoluteIncludePath );

                if ( !PreprocessForIncludesOnlyInternal( absoluteIncludePath ) )
                {
                    return false;
                }
            }
        }

        return true;
    }
};


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


static ShaderStage SpirvCrossShaderStageToPG( spv::ExecutionModel stage )
{
    switch( stage )
    {
    case spv::ExecutionModel::ExecutionModelVertex:
        return ShaderStage::VERTEX;
    case spv::ExecutionModel::ExecutionModelTessellationControl:
        return ShaderStage::TESSELLATION_CONTROL;
    case spv::ExecutionModel::ExecutionModelTessellationEvaluation:
        return ShaderStage::TESSELLATION_EVALUATION;
    case spv::ExecutionModel::ExecutionModelGeometry:
        return ShaderStage::GEOMETRY;
    case spv::ExecutionModel::ExecutionModelFragment:
        return ShaderStage::FRAGMENT;
    case spv::ExecutionModel::ExecutionModelGLCompute:
        return ShaderStage::COMPUTE;
    default:
        return ShaderStage::NUM_SHADER_STAGES;
    }
}


struct ShaderReflectData
{
    ShaderResourceLayout layout;
    std::string entryPoint;
    ShaderStage stage;
};


static void UpdateArraySize( const spirv_cross::SPIRType& type, ShaderResourceLayout& layout, unsigned set, unsigned binding )
{
	uint32_t& size = layout.sets[set].arraySizes[binding];
	if ( !type.array.empty() )
	{
		if ( type.array.size() != 1 )
        {
			LOG_ERR( "Array dimension must be 1.\n" );
        }
		else if (!type.array_size_literal.front())
        {
			LOG_ERR( "Array dimension must be a literal.\n" );
        }
		else
		{
			if ( type.array.front() == 0 )
			{
				// Unsized runtime array.
                LOG_ERR( "Currently not supporting bindless / descriptor indexing features\n" );
			}
			else if ( size && size != type.array.front() )
            {
				LOG_ERR( "Array dimension for set %u, binding %u is inconsistent.\n", set, binding );
            }
			else if ( type.array.front() + binding > PG_MAX_NUM_BINDINGS_PER_SET )
            {
				LOG_ERR( "Bindings in the array from set %u, binding %u, will go out of bounds. Max = %d\n", set, binding, PG_MAX_NUM_BINDINGS_PER_SET );
            }
            else
            {
				size = type.array.front();
            }
		}
	}
	else
	{
		if ( size && size != 1 )
        {
			LOG_ERR( "Array dimension for set %u, binding %u is inconsistent.\n", set, binding );
        }
		size = 1;
	}
}


static bool ReflectShaderSpirv( uint32_t* spirv, size_t sizeInBytes, ShaderReflectData& reflectData )
{
    using namespace spirv_cross;
    Compiler compiler( spirv, sizeInBytes / sizeof( uint32_t ) );

    const auto& entryPoints = compiler.get_entry_points_and_stages();
    if ( entryPoints.size() == 0 )
    {
        LOG_ERR( "No entry points found in shader!\n" );
        return false;
    }
    else if ( entryPoints.size() > 1 )
    {
        LOG_WARN( "Multiple entry points found in shader, but no selection method implemented yet. Using first entry '%s'\n", entryPoints[0].name.c_str() );
    }
    reflectData.entryPoint = entryPoints[0].name;
    reflectData.stage = SpirvCrossShaderStageToPG( entryPoints[0].execution_model );

	auto resources = compiler.get_shader_resources();
	for ( const auto& image : resources.sampled_images )
	{
		uint32_t set         = compiler.get_decoration( image.id, spv::DecorationDescriptorSet );
		uint32_t binding     = compiler.get_decoration( image.id, spv::DecorationBinding );
		const SPIRType& type = compiler.get_type( image.type_id );
		if ( type.image.dim == spv::DimBuffer )
        {
			reflectData.layout.sets[set].uniformBufferMask |= 1u << binding;
        }
		else
        {
			reflectData.layout.sets[set].sampledImageMask |= 1u << binding;
        }
		UpdateArraySize( type, reflectData.layout, set, binding );
	}
    
	for ( const Resource& image : resources.separate_images )
	{
		uint32_t set     = compiler.get_decoration( image.id, spv::DecorationDescriptorSet );
		uint32_t binding = compiler.get_decoration( image.id, spv::DecorationBinding );    
		const SPIRType& type = compiler.get_type( image.type_id );
		if ( type.image.dim == spv::DimBuffer )
        {
			reflectData.layout.sets[set].uniformBufferMask |= 1u << binding;
        }
        else
        {
			reflectData.layout.sets[set].separateImageMask |= 1u << binding;
        }
		UpdateArraySize( type, reflectData.layout, set, binding );
	}

    for ( const Resource& image : resources.storage_images )
	{
		uint32_t set     = compiler.get_decoration( image.id, spv::DecorationDescriptorSet );
		uint32_t binding = compiler.get_decoration( image.id, spv::DecorationBinding );
        const SPIRType& type = compiler.get_type( image.type_id );
		if ( type.image.dim == spv::DimBuffer )
        {
			reflectData.layout.sets[set].storageBufferMask |= 1u << binding;
        }
        else
        {
			reflectData.layout.sets[set].storageImageMask |= 1u << binding;
        }
		UpdateArraySize( type, reflectData.layout, set, binding );
	}

    for ( const Resource& buffer : resources.uniform_buffers )
	{
		uint32_t set     = compiler.get_decoration( buffer.id, spv::DecorationDescriptorSet );
		uint32_t binding = compiler.get_decoration( buffer.id, spv::DecorationBinding );
		reflectData.layout.sets[set].uniformBufferMask |= 1u << binding;
		UpdateArraySize( compiler.get_type( buffer.type_id ), reflectData.layout, set, binding );
	}
    
	for ( const Resource& buffer : resources.storage_buffers )
	{
		uint32_t set     = compiler.get_decoration( buffer.id, spv::DecorationDescriptorSet );
		uint32_t binding = compiler.get_decoration( buffer.id, spv::DecorationBinding );
		reflectData.layout.sets[set].storageBufferMask |= 1u << binding;
		UpdateArraySize( compiler.get_type( buffer.type_id ), reflectData.layout, set, binding );
	}
    
	for ( const Resource& sampler : resources.separate_samplers )
	{
		uint32_t set     = compiler.get_decoration( sampler.id, spv::DecorationDescriptorSet );
		uint32_t binding = compiler.get_decoration( sampler.id, spv::DecorationBinding );
		reflectData.layout.sets[set].samplerMask |= 1u << binding;
        UpdateArraySize( compiler.get_type( sampler.type_id ), reflectData.layout, set, binding );
	}

    for ( const Resource& attachmentImg : resources.subpass_inputs )
	{
		uint32_t set     = compiler.get_decoration( attachmentImg.id, spv::DecorationDescriptorSet );
		uint32_t binding = compiler.get_decoration( attachmentImg.id, spv::DecorationBinding );
		reflectData.layout.sets[set].inputAttachmentMask |= 1u << binding;
		UpdateArraySize( compiler.get_type( attachmentImg.type_id ), reflectData.layout, set, binding );
	}
   
	for ( const Resource& attrib : resources.stage_inputs )
	{
		uint32_t location = compiler.get_decoration( attrib.id, spv::DecorationLocation );
		reflectData.layout.inputMask |= 1u << location;
	}
    
	for ( const Resource& attrib : resources.stage_outputs)
	{
		uint32_t location = compiler.get_decoration( attrib.id, spv::DecorationLocation );
		reflectData.layout.outputMask |= 1u << location;
	}
 
	if ( !resources.push_constant_buffers.empty() )
	{
        const SPIRType& type = compiler.get_type( resources.push_constant_buffers.front().base_type_id );
		reflectData.layout.pushConstantSize = static_cast< uint32_t >( compiler.get_declared_struct_size( type ) );
	}

	const auto& specializationConstants = compiler.get_specialization_constants();
	for ( const auto &specConst : specializationConstants)
	{
        LOG_ERR( "Specialization constants not supported currently. Ignoring spec constant %u\n", specConst.constant_id );
	}

    return true;
}


static bool CompilePreprocessedShaderToSPIRV( const std::string& shaderFilename, shaderc_shader_kind kind, const std::string& shaderSource, const std::vector< std::pair< std::string, std::string > >& defines, std::vector< uint32_t >& outputSpirv )
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0 );
    options.SetSourceLanguage( shaderc_source_language_glsl );
#if USING( PG_OPTIMIZE_SHADERS )
    options.SetOptimizationLevel( shaderc_optimization_level_performance );
#else
    options.SetOptimizationLevel( shaderc_optimization_level_zero );
#endif // #if USING( PG_OPTIMIZE_SHADERS )
    
    options.AddMacroDefinition( "PG_SHADER_CODE", "1" );
    for ( const auto& [symbol, value] : defines )
    {
        options.AddMacroDefinition( symbol, value );
    }

    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( shaderSource, kind, shaderFilename.c_str(), options );
    if ( module.GetCompilationStatus() != shaderc_compilation_status_success )
    {
        LOG_ERR( "Compiling shader '%s' failed: '%s'\n", shaderFilename.c_str(), module.GetErrorMessage().c_str() );
        LOG( "Preprocessed shader for reference:\n---------------------------------------------\n" );
        LOG( "%s\n---------------------------------------------\n", shaderSource.c_str() );
        return false;
    }

    outputSpirv = { module.cbegin(), module.cend() };
    return true;
}


#if !USING( COMPILING_CONVERTER )
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
#endif // #if !USING( COMPILING_CONVERTER )


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

    ShaderPreprocessor preprocessor;
    if ( !preprocessor.Preprocess( createInfo.filename ) )
    {
        LOG_ERR( "Preprocessing for shader: name '%s', filename '%s' failed\n", createInfo.name.c_str(), createInfo.filename.c_str() );
        return false;
    }

    shaderc_shader_kind shadercShaderStage = PGToShadercShaderStage( createInfo.shaderStage );
    std::vector< uint32_t > spirv;
    if ( !CompilePreprocessedShaderToSPIRV( createInfo.filename, shadercShaderStage, preprocessor.outputShader, createInfo.defines, spirv ) )
    {
        return false;
    }

    size_t spirvSizeInBytes = 4 * spirv.size();
    ShaderReflectData reflectData;
    if ( !ReflectShaderSpirv( spirv.data(), spirvSizeInBytes, reflectData ) )
    {
        LOG_ERR( "Spirv reflection for shader: name '%s', filename '%s' failed\n", createInfo.name.c_str(), createInfo.filename.c_str() );
        return false;
    }
    PG_ASSERT( shader->shaderStage == reflectData.stage );
    shader->entryPoint = reflectData.entryPoint; // Todo: pointless, since shaderc assumes entry point is "main" for GLSL
    memcpy( &shader->resourceLayout, &reflectData.layout, sizeof( ShaderResourceLayout ) );

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
    serializer->Read( shader->entryPoint );
    serializer->Read( &shader->resourceLayout, sizeof( ShaderResourceLayout ) );
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
    serializer->Write( shader->entryPoint );
    serializer->Write( &shader->resourceLayout, sizeof( ShaderResourceLayout ) );

    serializer->Write( shader->spirv );
#endif // #else // #if !USING( COMPILING_CONVERTER )

    return true;
}


bool Shader_GetIncludes( const std::string& filename, ShaderStage shaderStage, std::vector< std::string >& includedFiles )
{
    ShaderPreprocessor preprocessor;
    if ( !preprocessor.ParseForIncludesOnly( filename ) )
    {
        LOG_ERR( "Preprocessing the includes for shader '%s' failed\n", filename.c_str() );
        return false;
    }
    includedFiles = std::move( preprocessor.includedFiles );

    return true;
}


} // namespace PG
