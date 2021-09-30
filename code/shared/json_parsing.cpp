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
    std::vector< char > buffer( fileSize );
    rapidjson::FileReadStream is( fp, buffer.data(), fileSize );

    rapidjson::ParseResult ok = document.ParseStream( is );
    if ( !ok )
    {
        LOG_ERR( "Failed to parse json file '%s'. Error: '%s' (%zu)", filename.c_str(), rapidjson::GetParseError_En( ok.Code() ), ok.Offset() );
        return false;
    }

    fclose( fp );
    return true;
}

glm::vec3 ParseVec3( const rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 3 );
    auto& GetF = ParseNumber< float >;
    return glm::vec3( GetF( v[0] ), GetF( v[1] ), GetF( v[2] ) );
}

glm::vec4 ParseVec4( const rapidjson::Value& v )
{
    PG_ASSERT( v.IsArray() && v.Size() == 4 );
    auto& GetF = ParseNumber< float >;
    return glm::vec4( GetF( v[0] ), GetF( v[1] ), GetF( v[2] ), GetF( v[3] ) );
}
