#include "base_asset.hpp"
#include "shared/serializer.hpp"

namespace PG
{

BaseAsset::~BaseAsset()
{
#if USING( ASSET_NAMES )
    if ( m_name )
        free( m_name );
#endif // #if USING( ASSET_NAMES )
}

BaseAsset::BaseAsset( std::string_view inName ) { SetName( inName ); }

void BaseAsset::SetName( std::string_view inName )
{
#if USING( ASSET_NAMES )
    if ( m_name )
        free( m_name );
    m_name = (char*)malloc( inName.length() + 1 );
    strcpy( m_name, inName.data() );
#else  // #if USING( ASSET_NAMES )
    PG_UNUSED( inName );
#endif // #else // #if USING( ASSET_NAMES )
}

const char* BaseAsset::GetName() const
{
#if USING( ASSET_NAMES )
    return m_name;
#else  // #if USING( ASSET_NAMES )
    return nullptr;
#endif // #else // #if USING( ASSET_NAMES )
}

void SerializeAssetMetadata( Serializer* serializer, const AssetMetadata& metadata )
{
    serializer->Write<u16>( metadata.name );
    serializer->Write( metadata.hash );
    serializer->Write( metadata.size );
}

AssetMetadata DeserializeAssetMetadata( Serializer* serializer )
{
    AssetMetadata metadata;
    serializer->Read<u16>( metadata.name );
    serializer->Read( metadata.hash );
    serializer->Read( metadata.size );

    return metadata;
}

} // namespace PG
