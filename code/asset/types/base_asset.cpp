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
#endif // #if USING( ASSET_NAMES )
}

const char* BaseAsset::GetName() const
{
#if USING( ASSET_NAMES )
    return m_name;
#else  // #if USING( ASSET_NAMES )
    return nullptr;
#endif // #else // #if USING( ASSET_NAMES )
}

void BaseAsset::SerializeName( Serializer* serializer ) const
{
#if USING( ASSET_NAMES )
    serializer->Write<u16>( m_name );
#endif // #if USING( ASSET_NAMES )
}

std::string DeserializeAssetName( Serializer* serializer, BaseAsset* asset )
{
    std::string assetName;
    u16 assetNameLen;
    serializer->Read( assetNameLen );
    assetName.resize( assetNameLen );
    serializer->Read( &assetName[0], assetNameLen );
    asset->SetName( assetName );

    return assetName;
}

} // namespace PG
