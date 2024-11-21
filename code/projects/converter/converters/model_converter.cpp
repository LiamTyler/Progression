#include "model_converter.hpp"
#include "asset/pmodel.hpp"
#include "asset/types/material.hpp"
#include "shared/hash.hpp"

namespace PG
{

void ModelConverter::AddReferencedAssetsInternal( ConstDerivedInfoPtr& modelInfo )
{
    auto matCreateInfo = std::make_shared<MaterialCreateInfo>();

    std::vector<std::string> materialNames = GetUsedMaterialsPModel( GetAbsPath_ModelFilename( modelInfo->filename ) );
    for ( const auto& matName : materialNames )
    {
        AddUsedAsset( ASSET_TYPE_MATERIAL, AssetDatabase::FindAssetInfo( ASSET_TYPE_MATERIAL, matName ) );
    }
}

std::string ModelConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash           = Hash( info->filename );
    HashCombine( hash, info->flipTexCoordsVertically );
    HashCombine( hash, info->recalculateNormals );
    HashCombine( hash, info->centerModel );
    HashCombine( hash, MAX_VERTS_PER_MESHLET );
    HashCombine( hash, MAX_TRIS_PER_MESHLET );
    cacheName += "_" + std::to_string( hash );

    return cacheName;
}

AssetStatus ModelConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    const std::string absPath = GetAbsPath_ModelFilename( info->filename );
    AddFastfileDependency( absPath );
    return IsFileOutOfDate( cacheTimestamp, absPath ) ? AssetStatus::OUT_OF_DATE : AssetStatus::UP_TO_DATE;
}

} // namespace PG
