#include "pipeline_converter.hpp"
#include "shared/hash.hpp"

namespace PG
{

void PipelineConverter::AddReferencedAssetsInternal( ConstDerivedInfoPtr& pipelineInfo )
{
    for ( const PipelineShaderInfo& shaderInfo : pipelineInfo->shaders )
    {
        auto shaderCI         = std::make_shared<ShaderCreateInfo>();
        shaderCI->name        = GetShaderCacheName( shaderInfo.name, shaderInfo.stage, pipelineInfo->defines );
        shaderCI->filename    = shaderInfo.name;
        shaderCI->shaderStage = shaderInfo.stage;
        shaderCI->defines     = pipelineInfo->defines;

        AddUsedAsset( ASSET_TYPE_SHADER, shaderCI );
    }

    if ( pipelineInfo->generateDebugPermutation )
    {
        auto debugPipelineCI          = std::make_shared<PipelineCreateInfo>();
        debugPipelineCI->name         = pipelineInfo->name + "_debug";
        debugPipelineCI->shaders      = pipelineInfo->shaders;
        debugPipelineCI->graphicsInfo = pipelineInfo->graphicsInfo;
        debugPipelineCI->defines      = pipelineInfo->defines;
        debugPipelineCI->defines.emplace_back( "IS_DEBUG_SHADER 1" );
        debugPipelineCI->generateDebugPermutation = false;

        AddUsedAsset( ASSET_TYPE_PIPELINE, debugPipelineCI );
    }
}

std::string PipelineConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash           = 0;
    for ( const PipelineShaderInfo& shaderInfo : info->shaders )
    {
        HashCombine( hash, shaderInfo.name );
        HashCombine( hash, shaderInfo.stage );
    }
    for ( const std::string& define : info->defines )
        HashCombine( hash, define );

    for ( const Gfx::PipelineColorAttachmentInfo& cAttach : info->graphicsInfo.colorAttachments )
    {
        HashCombine( hash, cAttach.format );
        HashCombine( hash, cAttach.blendMode );
    }

    static_assert( sizeof( Gfx::RasterizerInfo ) == 4 && sizeof( Gfx::PipelineDepthInfo ) == 4 );
    HashCombine( hash, *reinterpret_cast<const u32*>( &info->graphicsInfo.rasterizerInfo ) );
    HashCombine( hash, *reinterpret_cast<const u32*>( &info->graphicsInfo.depthInfo ) );
    HashCombine( hash, info->graphicsInfo.primitiveType );

    cacheName += "_" + std::to_string( hash );

    return cacheName;
}

AssetStatus PipelineConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    // if there is a cache entry at all, it should be up to date (since all fields are part of the hash name)
    return AssetStatus::UP_TO_DATE;
}

} // namespace PG
