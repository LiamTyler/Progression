#include "base_asset_converter.hpp"

namespace PG
{

ConverterConfigOptions g_converterConfigOptions;
ConverterStatus g_converterStatus;
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


void FilenameSlashesToUnderscores( std::string& str )
{
    for ( char& c : str )
    {
        if ( c == '/' || c == '\\' )
        {
            c = '_';
        }
    }
}


BaseAssetConverter::~BaseAssetConverter()
{
    for ( const auto& assetInfo : m_parsedAssets )
    {
        delete assetInfo;
    }
}


int BaseAssetConverter::ConvertAll()
{
    if ( m_outOfDateAssets.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( const auto& assetInfo : m_outOfDateAssets )
    {
        if ( !ConvertSingle( assetInfo ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


int BaseAssetConverter::CheckAllDependencies()
{
    int outOfDate = 0;
    for ( const auto& assetInfo : m_parsedAssets )
    {
        if ( IsAssetOutOfDate( assetInfo ) )
        {
            m_outOfDateAssets.push_back( assetInfo );
            ++outOfDate;
        }
    }

    return outOfDate;
}


bool BaseAssetConverter::BuildFastFile( Serializer* serializer ) const
{
    for ( size_t i = 0; i < m_parsedAssets.size(); ++i )
    {
        std::string ffiName = GetFastFileName( m_parsedAssets[i] );
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

} // namespace PG