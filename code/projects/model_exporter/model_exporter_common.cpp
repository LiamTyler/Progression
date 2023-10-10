#include "model_exporter_common.hpp"
#include "asset/asset_file_database.hpp"
#include "shared/filesystem.hpp"
#include <unordered_set>

using namespace PG;

std::string g_rootDir;

std::unordered_set<std::string> g_addedTexturesetNames;
std::unordered_set<std::string> g_addedMaterialNames;
std::unordered_set<std::string> g_addedModelNames;

vec2 AiToPG( const aiVector2D& v ) { return vec2( v.x, v.y ); }
vec3 AiToPG( const aiVector3D& v ) { return vec3( v.x, v.y, v.z ); }
vec4 AiToPG( const aiColor4D& v )  { return vec4( v.r, v.g, v.b, v.a ); }


static bool HaveCreated( AssetType assetType, const std::string& name )
{
    if ( assetType == ASSET_TYPE_TEXTURESET ) return g_addedTexturesetNames.contains( name );
    else if ( assetType == ASSET_TYPE_MATERIAL ) return g_addedMaterialNames.contains( name );
    else if ( assetType == ASSET_TYPE_MODEL ) return g_addedModelNames.contains( name );

    return false;
}


void AddCreatedName( AssetType assetType, const std::string& name )
{
    if ( assetType == ASSET_TYPE_TEXTURESET ) g_addedTexturesetNames.insert( name );
    else if ( assetType == ASSET_TYPE_MATERIAL ) g_addedMaterialNames.insert( name );
    else if ( assetType == ASSET_TYPE_MODEL ) g_addedModelNames.insert( name );
}


std::string GetUniqueAssetName( AssetType assetType, const std::string& name )
{
    std::string finalName = name;
    int postFix = 0;
    while ( AssetDatabase::FindAssetInfo( assetType, finalName ) || HaveCreated( assetType, finalName ) )
    {
        ++postFix;
        finalName = name + "_" + std::to_string( postFix );
    }

    if ( postFix != 0 )
        LOG_WARN( "%s already exist with name '%s'. Renaming to '%s'", g_assetNames[assetType], name.c_str(), finalName.c_str() );
    AddCreatedName( assetType, finalName );

    return finalName;
}


std::string Vec3ToJSON( const vec3& v )
{
    return "[ " + std::to_string( v.r ) + ", " + std::to_string( v.g ) + ", " + std::to_string( v.b ) + " ]";
}


void AddJSON( std::vector<std::string>& settings, const std::string& key, bool val )
{
    settings.push_back( "\"" + key + "\": " + (val ? "true" : "false") );
}

void AddJSON( std::vector<std::string>& settings, const std::string& key, const vec3& val )
{
    settings.push_back( "\"" + key + "\": " + Vec3ToJSON( val ) );
}

void AddJSON( std::vector<std::string>& settings, const std::string& key, const char* val )
{
    settings.push_back( "\"" + key + "\": \"" + std::string( val ) + "\"" );
}

void AddJSON( std::vector<std::string>& settings, const std::string& key, const std::string& val )
{
    settings.push_back( "\"" + key + "\": \"" + val + "\"" );
}


std::string GetPathRelativeToAssetDir( const std::string& path )
{
    std::string relPath = GetRelativePathToDir( path, PG_ASSET_DIR );
    if ( relPath.empty() )
    {
        relPath = path;
    }

    return relPath;
}