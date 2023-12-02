#include "asset/types/shader.hpp"
#include "asset/shader_preprocessor.hpp"
#include "core/feature_defines.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "shaderc/shaderc.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "spirv-tools/optimizer.hpp"
#include "spirv_cross/spirv_cross.hpp"
#if USING( CONVERTER )
    #include "converters/shader_converter.hpp"
#endif // #if USING( CONVERTER )
#include <fstream>

namespace PG
{

std::string GetAbsPath_ShaderFilename( const std::string& filename ) { return PG_ASSET_DIR "shaders/" + filename; }

#if USING( GPU_STRUCTS )

struct ShaderReflectData
{
    ShaderResourceLayout layout;
    std::string entryPoint;
    ShaderStage stage;
};

static ShaderStage SpirvCrossShaderStageToPG( spv::ExecutionModel stage )
{
    switch ( stage )
    {
    case spv::ExecutionModel::ExecutionModelVertex: return ShaderStage::VERTEX;
    case spv::ExecutionModel::ExecutionModelTessellationControl: return ShaderStage::TESSELLATION_CONTROL;
    case spv::ExecutionModel::ExecutionModelTessellationEvaluation: return ShaderStage::TESSELLATION_EVALUATION;
    case spv::ExecutionModel::ExecutionModelGeometry: return ShaderStage::GEOMETRY;
    case spv::ExecutionModel::ExecutionModelFragment: return ShaderStage::FRAGMENT;
    case spv::ExecutionModel::ExecutionModelGLCompute: return ShaderStage::COMPUTE;
    default: return ShaderStage::NUM_SHADER_STAGES;
    }
}

// returns true on ERROR
static bool ReflectShader_UpdateArraySize( const spirv_cross::SPIRType& type, ShaderResourceLayout& layout, unsigned set, unsigned binding )
{
    uint32_t& currentBindingSize = layout.sets[set].arraySizes[binding];
    if ( !type.array.empty() )
    {
        if ( type.array.size() != 1 )
        {
            LOG_ERR( "Array dimension must be 1." );
        }
        else if ( !type.array_size_literal.front() )
        {
            LOG_ERR( "Array dimension must be a literal." );
        }
        else
        {
            uint32_t reflectedSize = type.array.front();
            if ( reflectedSize == 0 )
            {
                // bindless
                if ( binding != 0 )
                {
                    LOG_ERR( "Bindless textures can only be used with binding = 0 in a set." );
                }
                if ( ( type.basetype != spirv_cross::SPIRType::Image && type.basetype != spirv_cross::SPIRType::SampledImage ) ||
                     type.image.dim == spv::DimBuffer )
                {
                    LOG_ERR( "Engine is currently only setup to use bindless images, not any other bindless type. TODO: allow other "
                             "bindless types" );
                }
                else
                {
                    currentBindingSize = UINT32_MAX;
                    return false;
                }
            }
            else if ( currentBindingSize == UINT32_MAX )
            {
                LOG( "\tAssuming array for set %u binding %u is trying to alias with the existing bindless resource at the same set & "
                     "binding",
                    set, binding );
                return false;
            }
            else if ( currentBindingSize && currentBindingSize != reflectedSize )
            {
                LOG_ERR( "Array dimension for set %u, binding %u is inconsistent. (Current %u, new %u)", set, binding, currentBindingSize,
                    reflectedSize );
            }
            else if ( type.array.front() + binding > PG_MAX_NUM_BINDINGS_PER_SET )
            {
                LOG_ERR( "Bindings in the array from set %u, binding %u, will go out of bounds. Max = %d", set, binding,
                    PG_MAX_NUM_BINDINGS_PER_SET );
            }
            else
            {
                currentBindingSize = reflectedSize;
                return false;
            }
        }
    }
    else
    {
        if ( currentBindingSize && currentBindingSize != 1 )
        {
            LOG_ERR( "Array dimension for set %u, binding %u is inconsistent. Non-array type has size > 1?", set, binding );
        }
        else
        {
            currentBindingSize = 1;
            return false;
        }
    }

    return true;
}

static bool ReflectShader_ReflectSpirv( uint32_t* spirv, size_t sizeInBytes, ShaderReflectData& reflectData )
{
    using namespace spirv_cross;

    spirv_cross::Compiler compiler( spirv, sizeInBytes / sizeof( uint32_t ) );
    const auto& entryPoints = compiler.get_entry_points_and_stages();
    if ( entryPoints.size() == 0 )
    {
        LOG_ERR( "No entry points found in shader!" );
        return false;
    }
    else if ( entryPoints.size() > 1 )
    {
        LOG_WARN( "Multiple entry points found in shader, but no selection method implemented yet. Using first entry '%s'",
            entryPoints[0].name.c_str() );
    }
    reflectData.entryPoint = entryPoints[0].name;
    reflectData.stage      = SpirvCrossShaderStageToPG( entryPoints[0].execution_model );

    uint32_t arrayErrors = 0;
    auto resources       = compiler.get_shader_resources();
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
        arrayErrors += ReflectShader_UpdateArraySize( type, reflectData.layout, set, binding );
    }

    for ( const Resource& image : resources.separate_images )
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
            reflectData.layout.sets[set].separateImageMask |= 1u << binding;
        }
        arrayErrors += ReflectShader_UpdateArraySize( type, reflectData.layout, set, binding );
    }

    for ( const Resource& image : resources.storage_images )
    {
        uint32_t set         = compiler.get_decoration( image.id, spv::DecorationDescriptorSet );
        uint32_t binding     = compiler.get_decoration( image.id, spv::DecorationBinding );
        const SPIRType& type = compiler.get_type( image.type_id );
        if ( type.image.dim == spv::DimBuffer )
        {
            reflectData.layout.sets[set].storageBufferMask |= 1u << binding;
        }
        else
        {
            reflectData.layout.sets[set].storageImageMask |= 1u << binding;
        }
        arrayErrors += ReflectShader_UpdateArraySize( type, reflectData.layout, set, binding );
    }

    for ( const Resource& buffer : resources.uniform_buffers )
    {
        uint32_t set     = compiler.get_decoration( buffer.id, spv::DecorationDescriptorSet );
        uint32_t binding = compiler.get_decoration( buffer.id, spv::DecorationBinding );
        reflectData.layout.sets[set].uniformBufferMask |= 1u << binding;
        arrayErrors += ReflectShader_UpdateArraySize( compiler.get_type( buffer.type_id ), reflectData.layout, set, binding );
    }

    for ( const Resource& buffer : resources.storage_buffers )
    {
        uint32_t set     = compiler.get_decoration( buffer.id, spv::DecorationDescriptorSet );
        uint32_t binding = compiler.get_decoration( buffer.id, spv::DecorationBinding );
        reflectData.layout.sets[set].storageBufferMask |= 1u << binding;
        arrayErrors += ReflectShader_UpdateArraySize( compiler.get_type( buffer.type_id ), reflectData.layout, set, binding );
    }

    for ( const Resource& sampler : resources.separate_samplers )
    {
        uint32_t set     = compiler.get_decoration( sampler.id, spv::DecorationDescriptorSet );
        uint32_t binding = compiler.get_decoration( sampler.id, spv::DecorationBinding );
        reflectData.layout.sets[set].samplerMask |= 1u << binding;
        arrayErrors += ReflectShader_UpdateArraySize( compiler.get_type( sampler.type_id ), reflectData.layout, set, binding );
    }

    for ( const Resource& attachmentImg : resources.subpass_inputs )
    {
        uint32_t set     = compiler.get_decoration( attachmentImg.id, spv::DecorationDescriptorSet );
        uint32_t binding = compiler.get_decoration( attachmentImg.id, spv::DecorationBinding );
        reflectData.layout.sets[set].inputAttachmentMask |= 1u << binding;
        arrayErrors += ReflectShader_UpdateArraySize( compiler.get_type( attachmentImg.type_id ), reflectData.layout, set, binding );
    }

    if ( arrayErrors > 0 )
    {
        return false;
    }

    for ( const Resource& attrib : resources.stage_inputs )
    {
        uint32_t location = compiler.get_decoration( attrib.id, spv::DecorationLocation );
        reflectData.layout.inputMask |= 1u << location;
    }

    for ( const Resource& attrib : resources.stage_outputs )
    {
        uint32_t location = compiler.get_decoration( attrib.id, spv::DecorationLocation );
        reflectData.layout.outputMask |= 1u << location;
    }

    if ( !resources.push_constant_buffers.empty() )
    {
        const SPIRType& type                = compiler.get_type( resources.push_constant_buffers.front().base_type_id );
        reflectData.layout.pushConstantSize = static_cast<uint32_t>( compiler.get_declared_struct_size( type ) );
    }

    const auto& specializationConstants = compiler.get_specialization_constants();
    for ( const auto& specConst : specializationConstants )
    {
        LOG_ERR( "Specialization constants not supported currently. Ignoring spec constant %u", specConst.constant_id );
    }

    return true;
}

static bool CompilePreprocessedShaderToSPIRV(
    const ShaderCreateInfo& createInfo, const std::string& shaderSource, std::vector<uint32_t>& outputSpirv )
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3 );
    options.SetSourceLanguage( shaderc_source_language_glsl );
    // If optimization is used, then spirv-opt gets called, and it actually crashes on windows.
    // Only seems to crash when using the binaries that come with Vulkan while also linking + using the spirv-cross lib.
    // Maybe spirv-cross and shaderc both link spirv-tools, that might be built differently? Some posts online say it doesnt do much either
    options.SetOptimizationLevel( shaderc_optimization_level_zero );
    if ( createInfo.generateDebugInfo )
    {
        options.SetGenerateDebugInfo();
    }

    shaderc_shader_kind kind             = static_cast<shaderc_shader_kind>( PGShaderStageToShaderc( createInfo.shaderStage ) );
    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv( shaderSource, kind, createInfo.filename.c_str(), options );
    if ( module.GetCompilationStatus() != shaderc_compilation_status_success )
    {
        LOG_ERR( "Compiling shader '%s' failed: '%s'", createInfo.filename.c_str(), module.GetErrorMessage().c_str() );
        return false;
    }
    else if ( module.GetNumWarnings() > 0 )
    {
        LOG_WARN( "Compiled shader '%s' had warnings: '%s'", createInfo.filename.c_str(), module.GetErrorMessage().c_str() );
    }

    outputSpirv = { module.cbegin(), module.cend() };

    return true;
}

#if USING( GPU_DATA )
static VkShaderModule CreateShaderModule( const uint32_t* spirv, size_t sizeInBytes )
{
    VkShaderModule module;
    VkShaderModuleCreateInfo vkShaderInfo = {};
    vkShaderInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vkShaderInfo.codeSize                 = sizeInBytes;
    vkShaderInfo.pCode                    = spirv;
    VkResult ret                          = vkCreateShaderModule( PG::Gfx::rg.device.GetHandle(), &vkShaderInfo, nullptr, &module );
    if ( ret != VK_SUCCESS )
    {
        LOG_ERR( "Could not create shader module from spirv" );
        return VK_NULL_HANDLE;
    }

    return module;
}
#endif // #if USING( GPU_DATA )

bool Shader::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    auto createInfo = (const ShaderCreateInfo*)baseInfo;

    name        = createInfo->name;
    shaderStage = createInfo->shaderStage;

    ShaderPreprocessOutput preproc = PreprocessShader( *createInfo, false );
    if ( !preproc.success )
    {
        return false;
    }

    std::vector<uint32_t> spirv;
    if ( !CompilePreprocessedShaderToSPIRV( *createInfo, preproc.outputShader, spirv ) )
    {
        return false;
    }

    size_t spirvSizeInBytes = 4 * spirv.size();
    ShaderReflectData reflectData;
    if ( !ReflectShader_ReflectSpirv( spirv.data(), spirvSizeInBytes, reflectData ) )
    {
        LOG_ERR( "Spirv reflection for shader: name '%s', filename '%s' failed", createInfo->name.c_str(), createInfo->filename.c_str() );
        return false;
    }
    PG_ASSERT( shaderStage == reflectData.stage );
    entryPoint = reflectData.entryPoint; // Todo: pointless, since shaderc assumes entry point is "main" for GLSL
    memcpy( &resourceLayout, &reflectData.layout, sizeof( ShaderResourceLayout ) );

#if USING( CONVERTER )
    savedSpirv = std::move( spirv );
    AddIncludeCacheEntry( cacheName, createInfo, preproc );
#elif USING( GPU_DATA ) // #if USING( CONVERTER )
    handle = CreateShaderModule( spirv.data(), 4 * spirv.size() );
    if ( handle == VK_NULL_HANDLE )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SHADER_NAME( this, name );
#endif // #elif USING( GPU_DATA ) // #if USING( CONVERTER )

    return true;
}

bool Shader::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );
    serializer->Read( shaderStage );
    serializer->Read( entryPoint );
    serializer->Read( &resourceLayout, sizeof( ShaderResourceLayout ) );
    std::vector<uint32_t> spirv;
    serializer->Read( spirv );

#if USING( GPU_DATA )
    handle = CreateShaderModule( spirv.data(), 4 * spirv.size() );
    if ( handle == VK_NULL_HANDLE )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SHADER_NAME( this, name );
#endif // #if USING( GPU_DATA )

    return true;
}

bool Shader::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
#if !USING( CONVERTER )
    PG_ASSERT( false, "Spirv code is only kept around in Converter builds for saving" );
#else // #if !USING( CONVERTER )
    serializer->Write( name );
    serializer->Write( shaderStage );
    serializer->Write( entryPoint );
    serializer->Write( &resourceLayout, sizeof( ShaderResourceLayout ) );
    serializer->Write( savedSpirv );
#endif // #else // #if !USING( CONVERTER )

    return true;
}

void Shader::Free()
{
#if USING( GPU_DATA )
    vkDestroyShaderModule( PG::Gfx::rg.device.GetHandle(), handle, nullptr );
#endif // #if USING( GPU_DATA )
}

#else // #if USING( GPU_STRUCTS )

bool Shader::Load( const BaseAssetCreateInfo* baseInfo ) { return false; }
bool Shader::FastfileLoad( Serializer* serializer ) { return false; }
bool Shader::FastfileSave( Serializer* serializer ) const { return false; }
void Shader::Free() {}

#endif // #else // #if USING( GPU_STRUCTS )

} // namespace PG
