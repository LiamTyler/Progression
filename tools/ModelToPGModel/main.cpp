#include "core/assert.hpp"
#include "asset/types/gfx_image.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "glm/glm.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace PG;

static void DisplayHelp()
{
    auto msg =
        "Usage: modelToPGModel MODEL_FILE NAME [texture search directory]\n"
        "\tOutputs the all files to ./NAME/\n"
        "\tTexture search dir defaults to the dir containing MODEL_FILE\n";

    std::cout << msg << std::endl;
}


static void Exit()
{
    Logger_Shutdown();
    exit( 0 );
}


static bool SaveMaterials( const std::string& filename, const aiScene* scene, std::vector< std::string >& materialNames, std::vector< GfxImageCreateInfo >& imageInfos );

static bool GetAssimpTexturePath( const aiMaterial* assimpMat, aiTextureType texType, std::string& pathToTex );


struct Mesh
{
    std::string name;
    int materialIndex = -1;
    uint32_t startIndex  = 0;
    uint32_t numIndices  = 0;
    uint32_t startVertex = 0;
    uint32_t numVertices = 0;
};

std::string g_name;
std::string g_outputDir;
std::string g_parentToOutputDir;
std::string g_textureSearchDir;

int main( int argc, char* argv[] )
{
    if ( argc < 3 )
    {
        DisplayHelp();
        Exit();
    }

    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    std::string filename = argv[1];
    g_name               = argv[2];
    g_outputDir          = GetAbsolutePath( GetParentPath( filename ) + g_name + "/" );
    g_parentToOutputDir  = GetParentPath( g_outputDir );
    if ( argc > 3 )
    {
        g_textureSearchDir = argv[3];
    }
    else
    {
        g_textureSearchDir = GetParentPath( GetAbsolutePath( filename ) );
    }
    LOG( "Filename = '%s'\n", GetAbsolutePath( filename ).c_str() );
    LOG( "Texture search directory = '%s'\n", g_textureSearchDir.c_str() );
    LOG( "Output directory = '%s'\n", g_outputDir.c_str() );
    LOG( "Output directory = '%s'\n", g_parentToOutputDir.c_str() );
    LOG( "Loading model...\n" );

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( filename.c_str(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
    if ( !scene )
    {
        LOG_ERR( "Error parsing model file '%s': '%s'\n", filename.c_str(), importer.GetErrorString() );
        Exit();
    }

    std::vector< Mesh > meshes;

    std::vector< glm::vec3 > vertices;
    std::vector< glm::vec3 > normals;
    std::vector< glm::vec2 > uvs;
    std::vector< glm::vec3 > tangents;
    std::vector< uint32_t > indices;
    meshes.resize( scene->mNumMeshes );
    uint32_t numVertices = 0;
    uint32_t numIndices = 0;
    uint32_t numVerticesWithUVs = 0;
    for ( size_t i = 0 ; i < meshes.size(); i++ )
    {
        meshes[i].name          = scene->mMeshes[i]->mName.C_Str();
        meshes[i].materialIndex = scene->mMeshes[i]->mMaterialIndex;
        if ( meshes[i].materialIndex < 0 )
        {
            LOG_ERR( "Mehs '%s' has invalid material index %d\n", meshes[i].materialIndex );
            Exit();
        }
        meshes[i].numIndices    = scene->mMeshes[i]->mNumFaces * 3;
        meshes[i].numVertices   = scene->mMeshes[i]->mNumVertices;
        meshes[i].startVertex   = numVertices;
        meshes[i].startIndex    = numIndices;
        
        numVertices += scene->mMeshes[i]->mNumVertices;
        numIndices  += meshes[i].numIndices;

        const aiMesh* paiMesh = scene->mMeshes[i];
        if ( paiMesh->HasTextureCoords( 0 ) )
        {
            numVerticesWithUVs += numVertices;
            if ( !paiMesh->HasTangentsAndBitangents() )
            {
                LOG_ERR( "Mesh '%s' has UVs but no tangents. Should never happen due to aiProcess_GenNormals and aiProcess_CalcTangentSpace!\n", meshes[i].name.c_str() );
                Exit();
            }
        }
    }

    if ( numVerticesWithUVs > 1000 && numVerticesWithUVs < 0.5f * numVertices )
    {
        float wastedMem = (numVertices - numVerticesWithUVs) * (5 * sizeof( float )) / 1024.0f / 1024.0f;
        LOG_WARN( "Model has only has some meshes with uvs, but not all. Zeros are added for vertices without them as a result, wasting %.3f MB\n", wastedMem );
    }

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

        for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
        {
            const aiVector3D* pPos    = &paiMesh->mVertices[vIdx];
            const aiVector3D* pNormal = &paiMesh->mNormals[vIdx];
            vertices.emplace_back( pPos->x, pPos->y, pPos->z );
            normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );

            if ( anyMeshHasUVs )
            {
                const aiVector3D* pTexCoord = &paiMesh->mTextureCoords[0][vIdx];
                uvs.emplace_back( pTexCoord->x, pTexCoord->y );

                const aiVector3D* pTangent = &paiMesh->mTangents[vIdx];
                glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                const glm::vec3& n = normals[vIdx];
                t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                tangents.emplace_back( t );
            }
            else
            {
                uvs.emplace_back( 0, 0 );
                tangents.emplace_back( 0, 0, 0 );
            }
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

    LOG( "Vertices: %d\n", vertices.size() );
    LOG( "Normals: %d\n", normals.size() );
    LOG( "uvs: %d\n", uvs.size() );
    LOG( "tangents: %d\n", tangents.size() );
    LOG( "indices: %d\n", indices.size() );
    LOG( "meshes: %d\n", meshes.size() );
    
    CreateDirectory( g_outputDir );
    CreateDirectory( g_outputDir + "textures/" );
    
    std::vector< std::string > materialNames = { "default" };
    std::vector< GfxImageCreateInfo > imageInfos;
    if ( !SaveMaterials( filename, scene, materialNames, imageInfos ) )
    {
        LOG_ERR( "Could not save the model's materials\n" );
        Exit();
    }

    std::string outputModelFilename = g_outputDir + GetFilenameStem( filename ) + ".pgModel";
    Serializer modelFile;
    if ( !modelFile.OpenForWrite( outputModelFilename ) )
    {
        LOG_ERR( "Could not open output model filename '%s'\n", outputModelFilename.c_str() );
        Exit();
    }
    modelFile.Write( vertices );
    modelFile.Write( normals );
    modelFile.Write( uvs );
    modelFile.Write( tangents );
    modelFile.Write( indices );
    modelFile.Write( meshes.size() );
    
    for ( size_t i = 0; i < meshes.size(); ++i )
    {
        const Mesh& mesh = meshes[i];
        modelFile.Write( mesh.name );
        modelFile.Write( materialNames[mesh.materialIndex] );
        modelFile.Write( mesh.startIndex );
        modelFile.Write( mesh.numIndices );
        modelFile.Write( mesh.startVertex );
        modelFile.Write( mesh.numVertices );
    }

    std::string imgTypes[] =
    {
        "TYPE_1D",
        "TYPE_1D_ARRAY",
        "TYPE_2D",
        "TYPE_2D_ARRAY",
        "TYPE_CUBEMAP",
        "TYPE_CUBEMAP_ARRAY",
        "TYPE_3D"
    };

    std::string imgSemantics[] =
    {
        "DIFFUSE",
        "NORMAL"
    };

    std::string assetListJson = "{ \"AssetList\":\n[";
    for ( size_t i = 0; i < imageInfos.size(); ++i )
    {
        const auto& info = imageInfos[i];
        assetListJson += "\n\t{ \"Image\": { ";
        assetListJson += "\"name\": \"" + info.name + "\", ";
        assetListJson += "\"filename\": \"" + info.filename + "\", ";
        assetListJson += "\"semantic\": \"" + imgSemantics[static_cast< int >( info.semantic )] + "\", ";
        assetListJson += "\"imageType\": \"" + imgTypes[static_cast< int >( info.imageType )] + "\" } },";
    }
    assetListJson += "\n\t{ \"MatFile\": { \"filename\": \"" + GetRelativePathToDir( g_outputDir + g_name + ".pgMtl", g_parentToOutputDir ) + "\" } },";
    assetListJson += "\n\t{ \"Model\": { \"name\": \"" + g_name + "\", \"filename\": \"" + GetRelativePathToDir( outputModelFilename, g_parentToOutputDir ) + "\" } }";
    assetListJson += "\n]}";
    std::string outputJson = g_outputDir + "assetList.json";
    std::ofstream out( outputJson );
    if ( !out )
    {
        LOG_ERR( "Could not open output pgMtl file '%s'\n", outputJson.c_str() );
        return false;
    }
    out << assetListJson;
    out.close();

    return 0;
}


bool SaveMaterials( const std::string& filename, const aiScene* scene, std::vector< std::string >& materialNames, std::vector< GfxImageCreateInfo >& imageInfos )
{
    std::string mtlJson = "{ \"Materials\":\n[";
    for ( uint32_t mtlIdx = 0; mtlIdx < scene->mNumMaterials; ++mtlIdx )
    {
        std::string name;
        glm::vec3 Kd = glm::vec3( 0 );
        std::string imagePath;
        std::string map_kd_name;

        aiString assimpMatName;
        aiColor3D color;

        const aiMaterial* assimpMat = scene->mMaterials[mtlIdx];
        assimpMat->Get( AI_MATKEY_NAME, assimpMatName );
        name = assimpMatName.C_Str();
        materialNames.push_back( name );

        color = aiColor3D( 0.f, 0.f, 0.f );
        assimpMat->Get( AI_MATKEY_COLOR_DIFFUSE, color );
        Kd = { color.r, color.g, color.b };

        // check if this is the default Assimp material
        if ( name == "DefaultMaterial" && glm::length( Kd - glm::vec3( .6f ) ) < 0.01f )
        {
            continue;
        }

        if ( assimpMat->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
        {
            if ( assimpMat->GetTextureCount( aiTextureType_DIFFUSE ) > 1 )
            {
                LOG_ERR( "Material '%s' has more than 1 diffuse texture\n", name.c_str() );
                return false;
            }
            if ( !GetAssimpTexturePath( assimpMat, aiTextureType_DIFFUSE, imagePath ) )
            {
                LOG_ERR( "Could not find diffuse texture\n" );
                return false;
            }
            map_kd_name = GetRelativePathToDir( GetFilenameMinusExtension( imagePath ), g_parentToOutputDir );
            GfxImageCreateInfo info;
            info.name       = map_kd_name;
            info.filename   = GetRelativePathToDir( imagePath, g_parentToOutputDir );
            info.semantic   = GfxImageSemantic::DIFFUSE;
            info.imageType  = GfxImageType::TYPE_2D;
            imageInfos.push_back( info );
        }

        mtlJson += "\n\t{ ";
        mtlJson += "\"name\": \"" + name + "\", ";
        mtlJson += "\"Kd\": [ " + std::to_string( Kd.r ) + ", " + std::to_string( Kd.g ) + ", " + std::to_string( Kd.b ) + " ]";
        if ( map_kd_name != "" )
        {
            mtlJson += ", \"map_Kd_name\": \"" + map_kd_name + "\"";
        }
        mtlJson += " }";
        if ( mtlIdx != scene->mNumMaterials - 1 )
        {
            mtlJson += ",";
        }
    }
    mtlJson += "\n]}";
    std::string outputFilename = g_outputDir + g_name + ".pgMtl";
    std::ofstream out( outputFilename );
    if ( !out )
    {
        LOG_ERR( "Could not open output pgMtl file '%s'\n", outputFilename.c_str() );
        return false;
    }
    out << mtlJson;
    out.close();

    return true;
}


static std::string TrimWhiteSpace( const std::string& s )
{
    size_t start = s.find_first_not_of( " \t" );
    size_t end   = s.find_last_not_of( " \t" );
    return s.substr( start, end - start + 1 );
}


bool GetAssimpTexturePath( const aiMaterial* assimpMat, aiTextureType texType, std::string& pathToTex )
{
    aiString path;
    namespace fs = std::filesystem;
    if ( assimpMat->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
    {
        std::string name = TrimWhiteSpace( path.data );

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
            LOG_ERR( "Could not find image file '%s'\n", name.c_str() );
            return false;
        }
        pathToTex = g_outputDir + "textures/" + GetRelativeFilename( fullPath );
        if ( !CopyFile( fullPath, pathToTex, true ) )
        {
            LOG_ERR( "Failed to copy image '%s' to '%s'\n", fullPath.c_str(), pathToTex.c_str() );
            return false;
        }
    }
    else
    {
        LOG_ERR( "Could not get texture of type: '%s'\n", TextureTypeToString( texType ) );
        return false;
    }

    return true;
}