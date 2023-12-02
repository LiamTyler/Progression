#include "shared/json_parsing.hpp"
#include "rapidjson/error/en.h"
#include "rapidjson/error/error.h"
#include "rapidjson/filereadstream.h"
#include "shared/logger.hpp"
#include <vector>

bool ParseJSONFile( const std::string& filename, rapidjson::Document& document )
{
    FILE* fp = fopen( filename.c_str(), "rb" );
    if ( fp == NULL )
    {
        LOG_ERR( "Could not open json file '%s'", filename.c_str() );
        return false;
    }
    fseek( fp, 0L, SEEK_END );
    auto fileSize = ftell( fp ) + 1;
    fseek( fp, 0L, SEEK_SET );
    std::vector<char> buffer( fileSize );
    rapidjson::FileReadStream is( fp, buffer.data(), fileSize );

    rapidjson::ParseResult ok = document.ParseStream( is );
    if ( !ok )
    {
        LOG_ERR(
            "Failed to parse json file '%s'. Error: '%s' (%zu)", filename.c_str(), rapidjson::GetParseError_En( ok.Code() ), ok.Offset() );
        return false;
    }

    fclose( fp );
    return true;
}
