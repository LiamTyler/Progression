#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "asset/asset_file_database.hpp"
#include "asset/types/pmodel_versions.hpp"
#include "asset/types/textureset.hpp"
#include "core/time.hpp"
#include "glm/glm.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/string.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>

using namespace PG;

std::string g_textureSearchDir;
std::atomic<uint32_t> g_warnings;

struct Mesh
{
    std::string name;
    uint32_t matIndex;
    uint32_t startIndex  = 0;
    uint32_t numIndices  = 0;
    uint32_t startVertex = 0;
    uint32_t numVertices = 0;
};

struct ImageInfo
{
    std::string name;
    std::string filename;
    std::string semantic;
    std::string type;
};

std::unordered_set<std::string> g_addedTexturesetNames;
std::unordered_set<std::string> g_addedMaterialNames;
std::unordered_set<std::string> g_addedModelNames;


bool HaveCreated( AssetType assetType, const std::string& name )
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


static std::string Vec3ToJSON( const glm::vec3& v )
{
    return "[ " + std::to_string( v.r ) + ", " + std::to_string( v.g ) + ", " + std::to_string( v.b ) + " ]";
}

template <typename T, typename = typename std::enable_if<std::is_arithmetic_v<T>, T>::type>
static void AddJSON( std::vector<std::string>& settings, const std::string& key, T val )
{
    settings.push_back( "\"" + key + "\": " + std::to_string( val ) );
}

static void AddJSON( std::vector<std::string>& settings, const std::string& key, bool val )
{
    settings.push_back( "\"" + key + "\": " + (val ? "true" : "false") );
}

static void AddJSON( std::vector<std::string>& settings, const std::string& key, const glm::vec3& val )
{
    settings.push_back( "\"" + key + "\": " + Vec3ToJSON( val ) );
}

static void AddJSON( std::vector<std::string>& settings, const std::string& key, const std::string& val )
{
    settings.push_back( "\"" + key + "\": \"" + val + "\"" );
}


static std::string GetPathRelativeToAssetDir( const std::string& path )
{
    std::string relPath = GetRelativePathToDir( path, PG_ASSET_DIR );
    if ( relPath.empty() )
    {
        relPath = path;
    }

    return relPath;
}

static bool GetAssimpTexturePath( const aiMaterial* assimpMat, aiTextureType texType, std::string& pathToTex )
{
    aiString path;
    namespace fs = std::filesystem;
    if ( assimpMat->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
    {
        std::string name = StripWhitespace( path.data );
        pathToTex = name;

        std::string fullPath;
        std::string basename = GetRelativeFilename( name );
        for( auto itEntry = fs::recursive_directory_iterator( g_textureSearchDir ); itEntry != fs::recursive_directory_iterator(); ++itEntry )
        {
            std::string itFile = itEntry->path().filename().string();
            if ( basename == itEntry->path().filename().string() )
            {
                fullPath = GetAbsolutePath( itEntry->path().string() );
                break;
            }
        }

        if ( fullPath == "" )
        {
            return false;
        }
        pathToTex = GetPathRelativeToAssetDir( fullPath );
    }
    else
    {
        LOG_ERR( "Could not get texture of type: '%s'", aiTextureTypeToString( texType ) );
        return false;
    }

    return true;
}

static std::optional<std::string> GetAssimpTexture( const aiMaterial* assimpMat, aiTextureType texType, const std::string& texTypeStr, const std::string& matName )
{
    std::string imagePath;
    if ( assimpMat->GetTextureCount( texType ) > 0 )
    {
        if ( assimpMat->GetTextureCount( texType ) > 1 )
        {
            LOG_WARN( "Material '%s' has more than 1 %s texture", matName.c_str(), texTypeStr.c_str() );
        }
        else if ( !GetAssimpTexturePath( assimpMat, texType, imagePath ) )
        {
            LOG_WARN( "Material '%s': can't find %s texture, using filename '%s'", matName.c_str(), texTypeStr.c_str(), imagePath.c_str() );
        }
        if ( !imagePath.empty() )
        {
            std::string relPath = GetRelativePathToDir( imagePath, PG_ASSET_DIR );
            if ( !relPath.empty() )
                imagePath = relPath;
        }
    }
    if ( !imagePath.empty() )
        return imagePath;
    return {};
}

static std::optional<glm::vec3> GetVec3( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    aiColor3D v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return glm::vec3( v.r, v.g, v.b );
    return {};
}

static std::optional<float> GetFloat( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    ai_real v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return (float)v;
    return {};
}

static std::optional<int> GetInt( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    ai_int v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return (int)v;
    return {};
}

static std::optional<std::string> GetString( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    aiString v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return std::string( v.C_Str() );
    return {};
}

static std::optional<bool> GetBool( const aiMaterial* assimpMat, const char* key, unsigned int type, unsigned int idx )
{
    bool v;
    if ( AI_SUCCESS == assimpMat->Get( key, type, idx, v ) )
        return v;
    return {};
}

static bool OutputMaterial( const aiMaterial* assimpMat, const aiScene* scene, const std::string& modelName, std::string& outputJSON )
{
    if ( !GetString( assimpMat, AI_MATKEY_NAME ) )
    {
        LOG_ERR( "Failed to parse material name while processing model '%s'", modelName.c_str() );
        return false;
    }
    std::string matName = GetUniqueAssetName( ASSET_TYPE_MATERIAL, GetString( assimpMat, AI_MATKEY_NAME )->c_str() );

    std::vector<std::string> matSettings;
    AddJSON( matSettings, "name", matName );

    if ( auto albedoTint = GetVec3( assimpMat, AI_MATKEY_BASE_COLOR ) )
        AddJSON( matSettings, "albedoTint", *albedoTint );

    if ( auto albedoTint = GetVec3( assimpMat, AI_MATKEY_COLOR_DIFFUSE ) )
        AddJSON( matSettings, "albedoTint", *albedoTint );

    auto metallicFactor = GetFloat( assimpMat, AI_MATKEY_METALLIC_FACTOR );
    if ( metallicFactor )
        AddJSON( matSettings, "metallicFactor", *metallicFactor );

    auto roughnessFactor = GetFloat( assimpMat, AI_MATKEY_ROUGHNESS_FACTOR );
    if ( roughnessFactor )
        AddJSON( matSettings, "roughnessFactor", *roughnessFactor );

    if ( auto emissiveTint = GetVec3( assimpMat, AI_MATKEY_COLOR_EMISSIVE ) )
    {
        if ( *emissiveTint != glm::vec3( 0 ) )
            AddJSON( matSettings, "emissiveTint", *emissiveTint );
    }

    // Textures
    {
        std::vector<std::string> texSettings;
        texSettings.push_back( "" ); // reserve slot for name (calculated later)

        if ( auto albedoMap = GetAssimpTexture( assimpMat, aiTextureType_BASE_COLOR, "baseColor", matName ) )
            AddJSON( texSettings, "albedoMap", *albedoMap );
        if ( auto albedoMap = GetAssimpTexture( assimpMat, aiTextureType_DIFFUSE, "diffuse", matName ) )
            AddJSON( texSettings, "albedoMap", *albedoMap );

        auto metalnessMap = GetAssimpTexture( assimpMat, aiTextureType_METALNESS, "metalness", matName );
        if ( metalnessMap )
            AddJSON( texSettings, "metalnessMap", *metalnessMap );
        else if ( metallicFactor )
            AddJSON( texSettings, "metalnessMap", "$white" );

        if ( auto normalMap = GetAssimpTexture( assimpMat, aiTextureType_NORMALS, "normal", matName ) )
            AddJSON( texSettings, "normalMap", *normalMap );

        auto roughnessMap = GetAssimpTexture( assimpMat, aiTextureType_SHININESS, "roughness", matName );
        if ( roughnessMap )
            AddJSON( texSettings, "roughnessMap", *roughnessMap );
        else if ( roughnessFactor )
            AddJSON( texSettings, "roughnessMap", "$white" );

        if ( auto emissiveMap = GetAssimpTexture( assimpMat, aiTextureType_EMISSIVE, "emissive", matName ) )
            AddJSON( texSettings, "emissiveMap", *emissiveMap );

        // Only name + output a textureset if it has non-default values
        if ( texSettings.size() > 1 )
        {
            std::string texturesetName = GetUniqueAssetName( ASSET_TYPE_TEXTURESET, matName );
            AddJSON( texSettings, "name", texturesetName );
            std::swap( texSettings[0], texSettings[texSettings.size() - 1] );
            texSettings.pop_back();
            if ( metalnessMap && metallicFactor )
            {
                LOG_WARN( "Textureset %s specifies both a metalness texture, and a metalness factor. This will probably look incorrect", texturesetName.c_str() );
                LOG_WARN( "\tbecause this engine uses the metalness factor as a multiply tint factor, and assumes a white texture when only a factor + no texture is specified" );
            }
            outputJSON += "\t{ \"Textureset\": {\n";
            for ( size_t i = 0; i < texSettings.size() - 1; ++i )
                outputJSON += "\t\t" + texSettings[i] + ",\n";
            outputJSON += "\t\t" + texSettings[texSettings.size() - 1] + "\n";
            outputJSON += "\t} },\n";
        }
    }

    outputJSON += "\t{ \"Material\": {\n";
    for ( size_t i = 0; i < matSettings.size() - 1; ++i )
        outputJSON += "\t\t" + matSettings[i] + ",\n";
    outputJSON += "\t\t" + matSettings[matSettings.size() - 1] + "\n";
    outputJSON += "\t} },\n";

#if 0
    std::vector<std::string> allMatSettings;
    if ( auto v = GetString( assimpMat, AI_MATKEY_NAME ) ) AddJSON( allMatSettings, "Name", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_TWOSIDED ) ) AddJSON( allMatSettings, "Two Sided", *v );
    if ( auto v = GetInt( assimpMat, AI_MATKEY_SHADING_MODEL ) ) AddJSON( allMatSettings, "Shading Model", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_ENABLE_WIREFRAME ) ) AddJSON( allMatSettings, "Enable Wireframe", *v );
    if ( auto v = GetInt( assimpMat, AI_MATKEY_BLEND_FUNC ) ) AddJSON( allMatSettings, "BlendFunc", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_OPACITY ) ) AddJSON( allMatSettings, "Opacity", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_TRANSPARENCYFACTOR ) ) AddJSON( allMatSettings, "Transparency Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_BUMPSCALING ) ) AddJSON( allMatSettings, "BumpScaling", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_SHININESS ) ) AddJSON( allMatSettings, "Shininess", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_REFLECTIVITY ) ) AddJSON( allMatSettings, "Reflectivity", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_SHININESS_STRENGTH ) ) AddJSON( allMatSettings, "Shininess Strength", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_REFRACTI ) ) AddJSON( allMatSettings, "Refracti", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_COLOR_DIFFUSE ) ) AddJSON( allMatSettings, "Diffuse", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_COLOR_AMBIENT ) ) AddJSON( allMatSettings, "Ambient", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_COLOR_SPECULAR ) ) AddJSON( allMatSettings, "Specular", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_COLOR_EMISSIVE ) ) AddJSON( allMatSettings, "Emissive", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_COLOR_TRANSPARENT ) ) AddJSON( allMatSettings, "Transparent", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_COLOR_REFLECTIVE ) ) AddJSON( allMatSettings, "Reflective", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_USE_COLOR_MAP ) ) AddJSON( allMatSettings, "Use Color Map", *v );
    if ( auto v = GetVec3( assimpMat, AI_MATKEY_BASE_COLOR ) ) AddJSON( allMatSettings, "Base Color", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_USE_METALLIC_MAP ) ) AddJSON( allMatSettings, "Use Metallic Map", *v );

    if ( auto v = GetFloat( assimpMat, AI_MATKEY_METALLIC_FACTOR ) ) AddJSON( allMatSettings, "Metallic Factor", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_USE_ROUGHNESS_MAP ) ) AddJSON( allMatSettings, "Use Roughness Map", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_ROUGHNESS_FACTOR ) ) AddJSON( allMatSettings, "Roughness Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_ANISOTROPY_FACTOR ) ) AddJSON( allMatSettings, "Anisotropy Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_SPECULAR_FACTOR ) ) AddJSON( allMatSettings, "Specular Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_GLOSSINESS_FACTOR ) ) AddJSON( allMatSettings, "Glossiness Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_SHEEN_COLOR_FACTOR ) ) AddJSON( allMatSettings, "Sheen Color Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_SHEEN_ROUGHNESS_FACTOR ) ) AddJSON( allMatSettings, "Sheen Roughness Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_CLEARCOAT_FACTOR ) ) AddJSON( allMatSettings, "Clearcoat Factor", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR ) ) AddJSON( allMatSettings, "Clearcoat Roughness Factor ", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_TRANSMISSION_FACTOR ) ) AddJSON( allMatSettings, "Transmission Factor", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_USE_EMISSIVE_MAP ) ) AddJSON( allMatSettings, "Use Emissive Map", *v );
    if ( auto v = GetFloat( assimpMat, AI_MATKEY_EMISSIVE_INTENSITY ) ) AddJSON( allMatSettings, "Emissive Intensity", *v );
    if ( auto v = GetBool( assimpMat, AI_MATKEY_USE_AO_MAP ) ) AddJSON( allMatSettings, "Use AO Map", *v );

    for ( size_t i = 0; i < allMatSettings.size(); ++i )
    {
        LOG( "%s", allMatSettings[i].c_str() );
    }

    for ( int i = 0; i < (int)AI_TEXTURE_TYPE_MAX; ++i )
    {
        auto t = (aiTextureType)(aiTextureType_DIFFUSE + i);
        int x = assimpMat->GetTextureCount( t );
        if ( x )
        {
            LOG( "%s: %d", aiTextureTypeToString( t ), x );
            std::string imagePath;
            GetAssimpTexturePath( assimpMat, t, imagePath );
            LOG( "\t%s", imagePath.c_str() );
        }
    }
#endif

    return true;
}


static bool ConvertModel( const std::string& filename, std::string& outputJSON )
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( filename.c_str(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
    if ( !scene )
    {
        LOG_ERR( "Error parsing model file '%s': '%s'", filename.c_str(), importer.GetErrorString() );
        return false;
    }

    std::vector<Mesh> meshes( scene->mNumMeshes );
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> tangents;
    std::vector<float> bitangentSigns;
    std::vector<uint32_t> indices;
    uint32_t numVertices = 0;
    uint32_t numIndices = 0;
    uint32_t numVerticesWithUVs = 0;
    std::string stem = GetFilenameStem( filename );

    std::vector<std::string> materialNames;
    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        std::string matName = scene->mMaterials[i]->GetName().C_Str();
        if ( matName.empty() || matName == "DefaultMaterial" )
        {
            matName = "default";
        }
        materialNames.push_back( matName );
    }

    for ( size_t i = 0 ; i < meshes.size(); i++ )
    {
        meshes[i].name = scene->mMeshes[i]->mName.C_Str();
        if ( meshes[i].name.empty() )
        {
            meshes[i].name = stem + "_mesh" + std::to_string( i );
        }
        meshes[i].matIndex = scene->mMeshes[i]->mMaterialIndex;
        PG_ASSERT( meshes[i].matIndex < materialNames.size() );
        meshes[i].numIndices  = scene->mMeshes[i]->mNumFaces * 3;
        meshes[i].numVertices = scene->mMeshes[i]->mNumVertices;
        meshes[i].startVertex = numVertices;
        meshes[i].startIndex  = numIndices;
        
        numVertices += scene->mMeshes[i]->mNumVertices;
        numIndices += meshes[i].numIndices;

        const aiMesh* paiMesh = scene->mMeshes[i];
        if ( paiMesh->HasTextureCoords( 0 ) )
        {
            numVerticesWithUVs += numVertices;
            if ( !paiMesh->HasTangentsAndBitangents() )
            {
                LOG_ERR( "Mesh '%s' has UVs but no tangents. Should never happen due to aiProcess_GenNormals and aiProcess_CalcTangentSpace!", meshes[i].name.c_str() );
                return false;
            }
        }
    }
    PG_ASSERT( numIndices % 3 == 0, "Was the model not triangulated?" );

    vertices.reserve( numVertices );
    normals.reserve( numVertices );
    uvs.reserve( numVertices );
    tangents.reserve( numVertices );
    indices.reserve( numIndices );
    bool anyMeshHasUVs = numVerticesWithUVs > 0;

    for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
    {
        const aiMesh* paiMesh = scene->mMeshes[meshIdx];
        const aiVector3D Zero3D( 0.0f, 0.0f, 0.0f );
        uint32_t nanTangents = 0;

        for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
        {
            const aiVector3D* pPos    = &paiMesh->mVertices[vIdx];
            const aiVector3D* pNormal = &paiMesh->mNormals[vIdx];
            glm::vec3 pos = { pPos->x, pPos->y, pPos->z };
            glm::vec3 normal = { pNormal->x, pNormal->y, pNormal->z };
            PG_ASSERT( !glm::any( glm::isnan( pos ) ) );
            PG_ASSERT( !glm::any( glm::isnan( normal ) ) );
            vertices.emplace_back( pos );
            normals.emplace_back( normal );
    
            if ( paiMesh->HasTextureCoords( 0 ) )
            {
                const aiVector3D* pTexCoord = &paiMesh->mTextureCoords[0][vIdx];
                glm::vec2 uv = { pTexCoord->x, pTexCoord->y };
                PG_ASSERT( !glm::any( glm::isnan( uv ) ) );
                uvs.emplace_back( uv );
    
                const aiVector3D* pTangent = &paiMesh->mTangents[vIdx];
                glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                t = glm::normalize( t - normal * glm::dot( normal, t ) ); // does assimp orthogonalize the tangents automatically?
                if ( glm::any( glm::isnan( t ) ) )
                {
                    ++nanTangents;
                    t = glm::vec3( 0 );
                }
                const aiVector3D* pBitangent = &paiMesh->mBitangents[vIdx];
                glm::vec3 bitangent( pBitangent->x, pBitangent->y, pBitangent->z );
                glm::vec3 calcBitangent = glm::cross( normal, t );
                float bDot = glm::dot( bitangent, calcBitangent );
                float sign = bDot >= 0.0f ? 1.0f : -1.0f;

                tangents.emplace_back( t );
                bitangentSigns.emplace_back( sign );
            }
            else if ( anyMeshHasUVs )
            {
                uvs.emplace_back( 0, 0 );
                tangents.emplace_back( 0, 0, 0 );
                bitangentSigns.push_back( 0 );
            }
        }
        if ( nanTangents > 0 )
        {
            LOG_WARN( "%s mesh %s has %u tangents that are NaN. Replacing with zeros", filename.c_str(), meshes[meshIdx].name.c_str(), nanTangents );
            ++g_warnings;
        }

        for ( size_t iIdx = 0; iIdx < paiMesh->mNumFaces; ++iIdx )
        {
            const aiFace& face = paiMesh->mFaces[iIdx];
            PG_ASSERT( face.mNumIndices == 3 );
            indices.push_back( face.mIndices[0] );
            indices.push_back( face.mIndices[1] );
            indices.push_back( face.mIndices[2] );
        }
    }
    
    LOG( "Model %s\n\tMeshes: %u, Materials: %u, Triangles: %u\n\tVertices: %u, Normals: %u, uvs: %u, tangents: %u",
        filename.c_str(), meshes.size(), materialNames.size(), indices.size() / 3, vertices.size(), normals.size(), uvs.size(), tangents.size() );

    std::string outputModelFilename = GetFilenameMinusExtension( filename ) + ".pmodel";
    std::ofstream outFile( outputModelFilename );
    if ( !outFile )
    {
        LOG_ERR( "Could not open output model filename '%s'", outputModelFilename.c_str() );
        return false;
    }
    outFile << "pmodelFormat: " << (uint32_t)PModelVersionNum::CURRENT_VERSION << "\n\n";

    outFile << "Materials: " << materialNames.size() << "\n";
    for ( size_t i = 0; i < materialNames.size(); ++i )
    {
        outFile << "" << i << ": " << materialNames[i] << "\n";
    }

    outFile << "Meshes: " << meshes.size() << "\n";
    for ( size_t i = 0; i < meshes.size(); ++i )
    {
        const Mesh& m = meshes[i];
        outFile << i << ": " << m.name << "\n";
        outFile << "    Mat: " << m.matIndex << "\n";
        outFile << "    StartTri: " << m.startIndex / 3 << "\n";
        outFile << "    StartVert: " << m.startVertex << "\n";
        outFile << "    NumTris: " << m.numIndices / 3 << "\n";
        outFile << "    NumVerts: " << m.numVertices << "\n";
    }

    outFile << "\nModelData:\n";

    outFile << std::fixed << std::setprecision( 6 );
    auto PrintVec3 = [&outFile]( glm::vec3 v ) { outFile << v.x << " " << v.y << " " << v.z << "\n"; };
    auto PrintVec2 = [&outFile]( glm::vec2 v ) { outFile << v.x << " " << v.y << "\n"; };

    outFile << "Positions: " << vertices.size() << "\n";
    for ( size_t i = 0; i < vertices.size(); ++i ) PrintVec3( vertices[i] );

    outFile << "Normals: " << normals.size() << "\n";
    for ( size_t i = 0; i < normals.size(); ++i ) PrintVec3( normals[i] );

    outFile << "TexCoords: " << uvs.size() << "\n";
    for ( size_t i = 0; i < uvs.size(); ++i ) PrintVec2( uvs[i] );

    outFile << "Tangents: " << tangents.size() << "\n";
    for ( size_t i = 0; i < tangents.size(); ++i )
    {
        const auto& t = tangents[i];
        outFile << t.x << " " << t.y << " " << t.z << (bitangentSigns[i] >= 0 ? " 1" : " -1") << "\n";
    }

    outFile << "Triangles: " << indices.size() / 3 << "\n";
    for ( size_t i = 0; i < indices.size(); i += 3 ) outFile << indices[i+0] << " " << indices[i+1] << " " << indices[i+2] << "\n";

    std::string modelName = GetFilenameStem( filename );
    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        if ( materialNames[i] != "default" )
        {
            OutputMaterial( scene->mMaterials[i], scene, modelName, outputJSON );
        }
    }

    std::string relPath = GetPathRelativeToAssetDir( outputModelFilename );
    outputJSON += "\t{ \"Model\": { \"name\": \"" + modelName + "\", \"filename\": \"" + relPath + "\" } },\n";

    return true;
}


static void DisplayHelp()
{
    auto msg =
        "Usage: modelToPGModel PATH [texture_search_dir]\n"
        "\tIf PATH is a directory, all models found in it (not recursive) are converted into .pmodel files.\n"
        "\tIf PATH is a file, then only that one file is converted to a pmodel.\n"
        "\tAlso creates an asset file (.paf) containing all the model, material, and texture info\n";
    std::cout << msg << std::endl;
}


int main( int argc, char* argv[] )
{
    if ( argc < 2 )
    {
        DisplayHelp();
        return 0;
    }

    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    g_warnings = 0;
    std::unordered_set<std::string> modelExtensions = { ".obj", ".fbx", ".ply", ".gltf", ".stl" };
    std::vector<std::string> filesToProcess;
    std::string directory;
    std::string outputPrefix;
    if ( IsDirectory( argv[1] ) )
    {
        directory = argv[1];
        g_textureSearchDir = argc > 2 ? argv[2] : directory;
        outputPrefix = GetDirectoryStem( directory );

        namespace fs = std::filesystem;
        for ( const auto& entry : fs::directory_iterator( directory ) )
        {
            std::string ext = GetFileExtension( entry.path().string() );
            if ( entry.is_regular_file() && modelExtensions.find( ext ) != modelExtensions.end() )
            {
                filesToProcess.push_back( entry.path().string() );
            }
        }
    }
    else if ( IsFile( argv[1] ) )
    {
        directory = g_textureSearchDir = GetParentPath( argv[1] );
        outputPrefix = GetFilenameStem( argv[1] );
        filesToProcess.push_back( argv[1] );
    }
    else
    {
        LOG_ERR( "Path '%s' does not exist!", argv[1] );
        return 0;
    }

    PG_ASSERT( directory.length() );
    if ( directory[directory.length() - 1] != '/' && directory[directory.length() - 1] != '\\' )
    {
        directory += '/';
    }

    // if we are re-exporting, delete before initializing the database, so that it doesn't affect the unique name generation
    std::string pafFilename = directory + "exported_" + outputPrefix + ".paf";
    if ( PathExists( pafFilename ) )
    {
        DeleteFile( pafFilename );
    }
    AssetDatabase::Init();

    auto startTime = Time::GetTimePoint();
    size_t modelsConverted = 0;
    std::string outputJSON = "[\n";
    outputJSON.reserve( 1024 * 1024 );
    // Cant run in parallel, since writing the output 
    // #pragma omp parallel for
    for ( int i = 0; i < static_cast<int>( filesToProcess.size() ); ++i )
    {
        std::string json;
        json.reserve( 2 * 1024 );
        modelsConverted += ConvertModel( filesToProcess[i], json );
        outputJSON += json;
    }

    LOG( "Models converted: %u in %.2f seconds", modelsConverted, Time::GetDuration( startTime ) / 1000.0f );
    LOG( "Errors: %zu, Warnings: %u", filesToProcess.size() - modelsConverted, g_warnings.load() );
    if ( modelsConverted > 0 )
    {
        outputJSON[outputJSON.length() - 2] = '\n';
        outputJSON[outputJSON.length() - 1] = ']';
        LOG( "Saving asset file %s", pafFilename.c_str() );
        std::ofstream out( pafFilename );
        out << outputJSON;
    }

    return 0;
}
