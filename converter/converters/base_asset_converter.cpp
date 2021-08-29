#include "base_asset_converter.hpp"
#include "asset_file_database.hpp"
#include "asset_cache.hpp"

namespace PG
{

ConverterConfigOptions g_converterConfigOptions;
static time_t s_latestAssetTimestamp;

void AddFastfileDependency( const std::string& file )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, GetFileTimestamp( file ) );
}


void ClearAllFastfileDependencies()
{
    s_latestAssetTimestamp = 0;
}


time_t GetLatestFastfileDependency()
{
    return s_latestAssetTimestamp;
}

/*
bool BaseAssetConverter::BuildFastFile( Serializer* serializer ) const
{
    for ( size_t i = 0; i < m_parsedAssets.size(); ++i )
    {
        std::string ffiName = GetCacheName( m_parsedAssets[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( m_assetType );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }
    return true;
}
*/

} // namespace PG