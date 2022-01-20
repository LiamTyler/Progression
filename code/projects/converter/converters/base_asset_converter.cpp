#include "base_asset_converter.hpp"

namespace PG
{

ConverterConfigOptions g_converterConfigOptions;
static time_t s_latestAssetTimestamp;

void AddFastfileDependency( const std::string& file )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, GetFileTimestamp( file ) );
}


void AddFastfileDependency( time_t timestamp )
{
    s_latestAssetTimestamp = std::max( s_latestAssetTimestamp, timestamp );
}


void ClearAllFastfileDependencies()
{
    s_latestAssetTimestamp = 0;
}


time_t GetLatestFastfileDependency()
{
    return s_latestAssetTimestamp;
}

} // namespace PG