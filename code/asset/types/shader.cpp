#include "asset/types/shader.hpp"
#include "asset/shader_preprocessor.hpp"
#include "core/feature_defines.hpp"
#include "shaderc/shaderc.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "spirv-tools/optimizer.hpp"
#include "spirv_cross/spirv_cross.hpp"
#include <fstream>

#if USING( CONVERTER )
#include "converters/shader_converter.hpp"
#endif // #if USING( CONVERTER )
#if USING( GPU_DATA )
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

std::string GetShaderCacheName( const std::string& filename, ShaderStage stage, const std::vector<std::string>& defines )
{
    size_t hash = 0;
    HashCombine( hash, stage );
    bool isDebug      = defines.empty() ? false : defines.back() == "IS_DEBUG_SHADER 1";
    size_t numDefines = isDebug ? defines.size() - 1 : defines.size();
    for ( size_t i = 0; i < numDefines; ++i )
        HashCombine( hash, defines[i] );

    if ( isDebug )
        return filename + "_d_" + std::to_string( hash );
    else
        return filename + "_" + std::to_string( hash );
}

std::string GetAbsPath_ShaderFilename( const std::string& filename ) { return PG_ASSET_DIR "shaders/" + filename; }

#if USING( GPU_STRUCTS )

static ShaderStage SpirvCrossShaderStageToPG( spv::ExecutionModel stage )
{
    switch ( stage )
    {
    case spv::ExecutionModel::ExecutionModelVertex: return ShaderStage::VERTEX;
    case spv::ExecutionModel::ExecutionModelGeometry: return ShaderStage::GEOMETRY;
    case spv::ExecutionModel::ExecutionModelFragment: return ShaderStage::FRAGMENT;
    case spv::ExecutionModel::ExecutionModelGLCompute: return ShaderStage::COMPUTE;
    case spv::ExecutionModel::ExecutionModelTaskEXT: return ShaderStage::TASK;
    case spv::ExecutionModel::ExecutionModelMeshEXT: return ShaderStage::MESH;
    default: return ShaderStage::NUM_SHADER_STAGES;
    }
    static_assert( Underlying( ShaderStage::NUM_SHADER_STAGES ) == 6, "update above" );
}

static bool ReflectShader_ReflectSpirv( uint32_t* spirv, size_t sizeInBytes, ShaderReflectData& reflectData )
{
    using namespace spirv_cross;

    spirv_cross::Compiler compiler( spirv, sizeInBytes / sizeof( uint32_t ) );

    reflectData.workgroupSize.x = compiler.get_execution_mode_argument( spv::ExecutionModeLocalSize, 0 );
    reflectData.workgroupSize.y = compiler.get_execution_mode_argument( spv::ExecutionModeLocalSize, 1 );
    reflectData.workgroupSize.z = compiler.get_execution_mode_argument( spv::ExecutionModeLocalSize, 2 );

    auto resources = compiler.get_shader_resources();
    if ( !resources.push_constant_buffers.empty() )
    {
        const SPIRType& type  = compiler.get_type( resources.push_constant_buffers.front().base_type_id );
        uint32_t lowestOffset = UINT32_MAX;
        for ( uint32_t i = 0; i < (uint32_t)type.member_types.size(); ++i )
        {
            lowestOffset = Min( lowestOffset, compiler.type_struct_member_offset( type, i ) );
        }
        uint32_t highestOffsetPlusSize = static_cast<uint32_t>( compiler.get_declared_struct_size( type ) );
        reflectData.pushConstantSize   = highestOffsetPlusSize - lowestOffset;
        reflectData.pushConstantOffset = lowestOffset;
        // reflectData.layout.pushConstantSize = static_cast<uint32_t>( compiler.get_declared_struct_size( type ) );
    }

    const auto& specializationConstants = compiler.get_specialization_constants();
    for ( const auto& specConst : specializationConstants )
    {
        LOG_WARN( "Specialization constants not supported currently. Ignoring spec constant %u", specConst.constant_id );
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
    options.SetOptimizationLevel( shaderc_optimization_level_performance );
    options.SetGenerateDebugInfo();

    // No entry point info needed, because GLSL mandates that the entry point is always void main()
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
    VkResult ret                          = vkCreateShaderModule( PG::Gfx::rg.device, &vkShaderInfo, nullptr, &module );
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

    name          = createInfo->name;
    m_shaderStage = createInfo->shaderStage;

    bool savePreproc = false;
#if USING( CONVERTER )
    savePreproc = g_converterConfigOptions.saveShaderPreproc;
#endif // #if USING( CONVERTER )

    ShaderPreprocessOutput preproc = PreprocessShader( *createInfo, savePreproc );
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
    if ( !ReflectShader_ReflectSpirv( spirv.data(), spirvSizeInBytes, m_reflectionData ) )
    {
        LOG_ERR( "Spirv reflection for shader: name '%s', filename '%s' failed", createInfo->name.c_str(), createInfo->filename.c_str() );
        return false;
    }

#if USING( CONVERTER )
    savedSpirv = std::move( spirv );
    AddIncludeCacheEntry( cacheName, createInfo, preproc );
#endif // #if USING( CONVERTER )
#if USING( GPU_DATA )
    m_handle = CreateShaderModule( spirv.data(), 4 * spirv.size() );
    if ( m_handle == VK_NULL_HANDLE )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SHADER_NAME( this->m_handle, name );
#endif // if USING( GPU_DATA )

    return true;
}

bool Shader::FastfileLoad( Serializer* serializer )
{
    PG_ASSERT( serializer );
    serializer->Read( name );
    serializer->Read( &m_reflectionData, sizeof( m_reflectionData ) );
    serializer->Read( m_shaderStage );
    std::vector<uint32_t> spirv;
    serializer->Read( spirv );

#if USING( GPU_DATA )
    m_handle = CreateShaderModule( spirv.data(), 4 * spirv.size() );
    if ( m_handle == VK_NULL_HANDLE )
    {
        return false;
    }
    PG_DEBUG_MARKER_SET_SHADER_NAME( this->m_handle, name );
#endif // #if USING( GPU_DATA )

    return true;
}

bool Shader::FastfileSave( Serializer* serializer ) const
{
    PG_ASSERT( serializer );
#if !USING( CONVERTER )
    PG_ASSERT( false, "Spirv code is only kept around in Converter builds for saving" );
#else  // #if !USING( CONVERTER )
    serializer->Write( name );
    serializer->Write( &m_reflectionData, sizeof( m_reflectionData ) );
    serializer->Write( m_shaderStage );
    serializer->Write( savedSpirv );
#endif // #else // #if !USING( CONVERTER )

    return true;
}

void Shader::Free()
{
#if USING( GPU_DATA )
    vkDestroyShaderModule( PG::Gfx::rg.device, m_handle, nullptr );
#endif // #if USING( GPU_DATA )
}

#else // #if USING( GPU_STRUCTS )

bool Shader::Load( const BaseAssetCreateInfo* baseInfo ) { return false; }
bool Shader::FastfileLoad( Serializer* serializer ) { return false; }
bool Shader::FastfileSave( Serializer* serializer ) const { return false; }
void Shader::Free() {}

#endif // #else // #if USING( GPU_STRUCTS )

} // namespace PG
