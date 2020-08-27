#include "assert.hpp"
#include "converters/gfx_image_converter.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include "utils/json_parsing.hpp"
#include <chrono>

bool g_parsingError;

void RunConverter( const std::string& assetFile )
{
    CreateDirectory( PG_ASSET_DIR "cache/" );
    CreateDirectory( PG_ASSET_DIR "cache/images/" );

    auto converterStartTime = std::chrono::system_clock::now();
    LOG( "Loading asset list file '%s'...\n", assetFile.c_str() );
    rapidjson::Document document;
    if ( !ParseJSONFile( assetFile, document ) )
    {
        return;
    }
    
    static FunctionMapper< void > mapping(
    {
        { "Image",   GfxImage_Parse },
    });
    
    g_parsingError = false;
    mapping.ForEachMember( document );
    if ( g_parsingError )
    {
        LOG_ERR( "Parsing errors, exiting\n" );
        return;
    }

    LOG( "Checking asset dependencies...\n" );
    int numGfxImageOutOfDate = GfxImage_CheckDependencies();
    LOG( "GfxImages out of date: %d\n", numGfxImageOutOfDate );

    int totalNumAssetsOutOfDate = numGfxImageOutOfDate;
    if ( totalNumAssetsOutOfDate == 0 )
    {
        LOG( "All assets up to date\n" );
    }
    else
    {
        LOG( "Converting assets...\n" );
        int numGfxImageConvertErrors = GfxImage_Convert();

        int totalErrors = numGfxImageConvertErrors;
        if ( totalErrors )
        {
            LOG_ERR( "%d errors while converting, convert failed\n", totalErrors );
        }
        else
        {
            LOG( "SUCCESS\n" );
        }
    }

    float duration = std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - converterStartTime ).count() / 1e6f;
    LOG( "Converter finished in %.2f seconds\n", duration );
}


int main( int argc, char** argv )
{
    if ( argc != 2 )
    {
        printf( "Usage: converter [path_to_assetlist.json]\n" );
        return 0;
    }

    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    
    RunConverter( argv[1] );

    Logger_Shutdown();
    return 0;
}