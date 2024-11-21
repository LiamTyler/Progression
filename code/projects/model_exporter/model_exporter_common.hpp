#pragma once

#include "asset/asset_versions.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "shared/assert.hpp"
#include "shared/logger.hpp"
#include "shared/math_vec.hpp"
#include <vector>

struct Options
{
    std::string rootDir;
    bool ignoreNameCollisions;
    bool printModelInfo;
    u32 floatPrecision;
};

extern Options g_options;

vec2 AiToPG( const aiVector2D& v );
vec3 AiToPG( const aiVector3D& v );
vec4 AiToPG( const aiColor4D& v );

void AddCreatedName( AssetType assetType, const std::string& name );

std::string GetUniqueAssetName( AssetType assetType, const std::string& name );

std::string Vec3ToJSON( const vec3& v );

template <typename T, typename = typename std::enable_if<std::is_arithmetic_v<T>, T>::type>
static void AddJSON( std::vector<std::string>& settings, const std::string& key, T val )
{
    settings.push_back( "\"" + key + "\": " + std::to_string( val ) );
}

void AddJSON( std::vector<std::string>& settings, const std::string& key, bool val );
void AddJSON( std::vector<std::string>& settings, const std::string& key, const vec3& val );
void AddJSON( std::vector<std::string>& settings, const std::string& key, const char* val );
void AddJSON( std::vector<std::string>& settings, const std::string& key, const std::string& val );
