#include "model_exporter_common.hpp"
#include "model_exporter_materials.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/pmodel.hpp"
#include "asset/types/textureset.hpp"
#include "core/time.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/string.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace PG;
using namespace glm;

static_assert( PMODEL_MAX_UVS_PER_VERT == AI_MAX_NUMBER_OF_TEXTURECOORDS );
static_assert( PMODEL_MAX_COLORS_PER_VERT == AI_MAX_NUMBER_OF_COLOR_SETS );

std::atomic<uint32_t> g_warnings;

static mat4 AiToGLMMat4( const aiMatrix4x4& aiM )
{
    mat4 m;
    m[0] = vec4( aiM.a1, aiM.b1, aiM.c1, aiM.d1 );
    m[1] = vec4( aiM.a2, aiM.b2, aiM.c2, aiM.d2 );
    m[2] = vec4( aiM.a3, aiM.b3, aiM.c3, aiM.d3 );
    m[3] = vec4( aiM.a4, aiM.b4, aiM.c4, aiM.d4 );

    return m;
}

static void ProcessVertices( const aiMesh* paiMesh, const mat4& localToWorldMat, PModel::Mesh& pgMesh )
{
    // it doesn't look like assimp guarantees that the uv or color sets will be contiguous. Aka
    // there could be uv0 and uv2 but no uv1. I want to compact them contiguously though
    uint32_t uvSetRemap[AI_MAX_NUMBER_OF_TEXTURECOORDS] = { UINT_MAX };
    uint32_t colorSetRemap[AI_MAX_NUMBER_OF_COLOR_SETS] = { UINT_MAX };
    static_assert( AI_MAX_NUMBER_OF_TEXTURECOORDS == 8 );
    {
        uint32_t pgUVIdx = 0;
        uint32_t pgColorIdx = 0;
        for ( uint32_t i = 0; i < 8; ++i )
        {
            if ( paiMesh->HasTextureCoords( i ) )
            {
                uvSetRemap[pgUVIdx++] = i;
            }
            if ( paiMesh->HasVertexColors( i ) )
            {
                colorSetRemap[pgColorIdx++] = i;
            }
        }
    }

    mat4 normalMatrix = inverse( transpose( localToWorldMat ) );

    uint32_t numVerts = paiMesh->mNumVertices;
    pgMesh.vertices.resize( numVerts );
    for ( uint32_t vIdx = 0; vIdx < numVerts; ++vIdx )
    {
        PModel::Vertex& v = pgMesh.vertices[vIdx];
        v.pos = AiToPG( paiMesh->mVertices[vIdx] );
        v.pos = vec3( localToWorldMat * vec4( v.pos, 1.0f ) );
        v.normal = AiToPG( paiMesh->mNormals[vIdx] );
        v.normal = vec3( normalMatrix * vec4( v.normal, 0.0f ) );
        PG_ASSERT( !any( isnan( v.pos ) ) );
        PG_ASSERT( !any( isnan( v.normal ) ) );
        v.numBones = 0;

        if ( pgMesh.hasTangents )
        {
            v.tangent = AiToPG( paiMesh->mTangents[vIdx] );
            v.tangent = vec3( localToWorldMat * vec4( v.tangent, 0.0f ) );
            v.bitangent = AiToPG( paiMesh->mBitangents[vIdx] );
            v.bitangent = vec3( localToWorldMat * vec4( v.bitangent, 0.0f ) );
        }

        for ( uint32_t uvSetIdx = 0; uvSetIdx < pgMesh.numUVChannels; ++uvSetIdx )
        {
            uint32_t aiUVSetIdx = uvSetRemap[uvSetIdx];
            vec3 uvw = AiToPG( paiMesh->mTextureCoords[aiUVSetIdx][vIdx] );
            v.uvs[uvSetIdx] = vec2( uvw.x, uvw.y );
        }

        for ( uint32_t colorSetIdx = 0; colorSetIdx < pgMesh.numColorChannels; ++colorSetIdx )
        {
            uint32_t aiColorSetIdx = colorSetRemap[colorSetIdx];
            v.colors[colorSetIdx] = AiToPG( paiMesh->mColors[aiColorSetIdx][vIdx] );
        }
    }

    std::vector<uint32_t> numBonesPerVertex( numVerts, 0 );
    for ( uint32_t aiBoneIdx = 0; aiBoneIdx < paiMesh->mNumBones; ++aiBoneIdx )
    {
        const aiBone& paiBone = *paiMesh->mBones[aiBoneIdx];
        for ( uint32_t weightIdx = 0; weightIdx < paiBone.mNumWeights; ++weightIdx )
        {
            const aiVertexWeight& w = paiBone.mWeights[weightIdx];
            PG_ASSERT( w.mVertexId < numVerts );
            numBonesPerVertex[w.mVertexId]++;
            pgMesh.vertices[w.mVertexId].AddBone( aiBoneIdx, w.mWeight );
        }
    }

    for ( uint32_t vIdx = 0; vIdx < numVerts; ++vIdx )
    {
        if ( numBonesPerVertex[vIdx] > PMODEL_MAX_BONE_WEIGHTS_PER_VERT )
        {
            LOG_WARN( "Vertex %u in mesh %s has %u bones weights in the source file, but Pmodels only support %u per vertex. Using the %u highest weights instead",
                vIdx, pgMesh.name.c_str(), numBonesPerVertex[vIdx], PMODEL_MAX_BONE_WEIGHTS_PER_VERT, PMODEL_MAX_BONE_WEIGHTS_PER_VERT );
        }

        PModel::Vertex& v = pgMesh.vertices[vIdx];
        float weightTotal = 0;
        for ( uint8_t slot = 0; slot < v.numBones; ++slot )
            weightTotal += v.boneWeights[slot];

        for ( uint8_t slot = 0; slot < v.numBones; ++slot )
            v.boneWeights[slot] /= weightTotal;
    }
}


void ParseNode( const std::string& filename, const std::vector<std::string>& materialNames, const aiScene* scene, const aiNode* node, const mat4& parentLocalToWorld, PModel& pmodel )
{
    std::string stem = GetFilenameStem( filename );
    mat4 currentLocalToWorld = AiToGLMMat4( node->mTransformation ) * parentLocalToWorld;
    for ( uint32_t meshIdx = 0; meshIdx < node->mNumMeshes; ++meshIdx )
    {
        const aiMesh* paiMesh = scene->mMeshes[node->mMeshes[meshIdx]];
        PModel::Mesh& pgMesh = pmodel.meshes.emplace_back();
        pgMesh.name = paiMesh->mName.C_Str();
        if ( pgMesh.name.empty() )
        {
            pgMesh.name = stem + "_mesh" + std::to_string( meshIdx );
            LOG_WARN( "Mesh %u in file %s does not have a name. Assigning name %s", meshIdx, filename.c_str(), pgMesh.name.c_str() );
        }
        pgMesh.materialName = materialNames[paiMesh->mMaterialIndex];
        pgMesh.numColorChannels = paiMesh->GetNumColorChannels();
        pgMesh.numUVChannels = paiMesh->GetNumUVChannels();
        pgMesh.hasTangents = paiMesh->mTangents && paiMesh->mBitangents;
        pgMesh.hasBoneWeights = paiMesh->HasBones();

        if ( paiMesh->mNumAnimMeshes )
        {
            LOG_WARN( "Mesh %u '%s' in file %s contains attachment/anim meshes. Currently not supported",
                meshIdx, filename.c_str(), pgMesh.name.c_str() );
        }

        for ( uint32_t uvSetIdx = 0; uvSetIdx < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++uvSetIdx )
        {
            if ( !paiMesh->HasTextureCoords( uvSetIdx ) || paiMesh->mNumUVComponents[uvSetIdx] == 2 )
                continue;

            if ( paiMesh->HasTextureCoordsName( uvSetIdx ) )
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u '%s' has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->GetTextureCoordsName( uvSetIdx )->C_Str(), paiMesh->mNumUVComponents[uvSetIdx] );
            }
            else
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u (unnamed) has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->mNumUVComponents[uvSetIdx] );
            }
        }

        ProcessVertices( paiMesh, currentLocalToWorld, pgMesh );

        pgMesh.indices.resize( paiMesh->mNumFaces * 3 );
        for ( uint32_t faceIdx = 0; faceIdx < paiMesh->mNumFaces; ++faceIdx )
        {
            const aiFace& face = paiMesh->mFaces[faceIdx];
            pgMesh.indices[faceIdx * 3 + 0] = face.mIndices[0];
            pgMesh.indices[faceIdx * 3 + 1] = face.mIndices[1];
            pgMesh.indices[faceIdx * 3 + 2] = face.mIndices[2];
        }
    }

    for ( uint32_t childIdx = 0; childIdx < node->mNumChildren; ++childIdx )
    {
        ParseNode( filename, materialNames, scene, node->mChildren[childIdx], currentLocalToWorld, pmodel );
    }
}


static bool ConvertModel( const std::string& filename, std::string& outputJSON )
{
    LOG( "Parsing file %s...", filename.c_str() );
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile( filename.c_str(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
    if ( !scene )
    {
        LOG_ERR( "Error parsing model file '%s': '%s'", filename.c_str(), importer.GetErrorString() );
        return false;
    }

    std::vector<std::string> materialNames;
    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        std::string matName = scene->mMaterials[i]->GetName().C_Str();
        if ( matName == "DefaultMaterial" || matName == "default" )
            matName = "default";
        else if ( matName == "" )
            matName = "material_" + std::to_string( i );

        materialNames.push_back( matName );
    }

    std::string modelName = GetFilenameStem( filename );

    MaterialContext matContext;
    matContext.scene = scene;
    matContext.file = filename;
    matContext.modelName = modelName;
    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        if ( materialNames[i] != "default" )
        {
            matContext.assimpMat = scene->mMaterials[i];
            matContext.localMatName = materialNames[i];
            OutputMaterial( matContext, outputJSON );
        }
    }

    PModel pmodel;

    aiNode* root = scene->mRootNode;
    ParseNode( filename, materialNames, scene, scene->mRootNode, mat4( 1.0f ), pmodel );


    /*
    pmodel.meshes.resize( scene->mNumMeshes );
    std::string stem = GetFilenameStem( filename );
    for ( uint32_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx )
    {
        const aiMesh* paiMesh = scene->mMeshes[meshIdx];
        PModel::Mesh& pgMesh = pmodel.meshes[meshIdx];
        pgMesh.name = paiMesh->mName.C_Str();
        if ( pgMesh.name.empty() )
        {
            pgMesh.name = stem + "_mesh" + std::to_string( meshIdx );
            LOG_WARN( "Mesh %u in file %s does not have a name. Assigning name %s", meshIdx, filename.c_str(), pgMesh.name.c_str() );
        }
        pgMesh.materialName = materialNames[paiMesh->mMaterialIndex];
        pgMesh.numColorChannels = paiMesh->GetNumColorChannels();
        pgMesh.numUVChannels = paiMesh->GetNumUVChannels();
        pgMesh.hasTangents = paiMesh->mTangents && paiMesh->mBitangents;
        pgMesh.hasBoneWeights = paiMesh->HasBones();

        if ( paiMesh->mNumAnimMeshes )
        {
            LOG_WARN( "Mesh %u '%s' in file %s contains attachment/anim meshes. Currently not supported",
                meshIdx, filename.c_str(), pgMesh.name.c_str() );
        }

        for ( uint32_t uvSetIdx = 0; uvSetIdx < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++uvSetIdx )
        {
            if ( !paiMesh->HasTextureCoords( uvSetIdx ) || paiMesh->mNumUVComponents[uvSetIdx] == 2 )
                continue;

            if ( paiMesh->HasTextureCoordsName( uvSetIdx ) )
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u '%s' has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->GetTextureCoordsName( uvSetIdx )->C_Str(), paiMesh->mNumUVComponents[uvSetIdx] );
            }
            else
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u (unnamed) has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->mNumUVComponents[uvSetIdx] );
            }
        }

        ProcessVertices( paiMesh, pgMesh );

        pgMesh.indices.resize( paiMesh->mNumFaces * 3 );
        for ( uint32_t faceIdx = 0; faceIdx < paiMesh->mNumFaces; ++faceIdx )
        {
            const aiFace& face = paiMesh->mFaces[faceIdx];
            pgMesh.indices[faceIdx * 3 + 0] = face.mIndices[0];
            pgMesh.indices[faceIdx * 3 + 1] = face.mIndices[1];
            pgMesh.indices[faceIdx * 3 + 2] = face.mIndices[2];
        }
    }
    */

    size_t totalVerts = 0;
    size_t totalTris = 0;
    for ( const PModel::Mesh& mesh : pmodel.meshes )
    {
        totalVerts += mesh.vertices.size();
        totalTris += mesh.indices.size() / 3;
    }
    
    LOG( "Model %s\n\tMeshes: %u, Materials: %u, Triangles: %u\n\tVertices: %u",
        filename.c_str(), pmodel.meshes.size(), materialNames.size(), totalTris, totalVerts );

    std::string outputModelFilename = GetFilenameMinusExtension( filename ) + ".pmodel";
    if ( !pmodel.Save( outputModelFilename, true ) )
        return false;

    std::string relPath = GetRelativePathToDir( outputModelFilename, g_rootDir );
    outputJSON += "\t{ \"Model\": { \"name\": \"" + modelName + "\", \"filename\": \"" + relPath + "\" } },\n";

    LOG( "Done processing file %s", filename.c_str() );

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
    char buff[64] = { 0 };
    float f = 0.0f;
    sprintf( buff, "%.6g", f );
    
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
    std::string outputPrefix;
    if ( IsDirectory( argv[1] ) )
    {
        g_rootDir = argv[1];
        g_textureSearchDir = argc > 2 ? argv[2] : g_rootDir;
        outputPrefix = GetDirectoryStem( g_rootDir );

        namespace fs = std::filesystem;
        for ( const auto& entry : fs::directory_iterator( g_rootDir ) )
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
        g_rootDir = g_textureSearchDir = GetParentPath( argv[1] );
        outputPrefix = GetFilenameStem( argv[1] );
        filesToProcess.push_back( argv[1] );
    }
    else
    {
        LOG_ERR( "Path '%s' does not exist!", argv[1] );
        return 0;
    }

    PG_ASSERT( g_rootDir.length() );
    if ( g_rootDir[g_rootDir.length() - 1] != '/' && g_rootDir[g_rootDir.length() - 1] != '\\' )
    {
        g_rootDir += '/';
    }

    // if we are re-exporting, delete before initializing the database, so that it doesn't affect the unique name generation
    std::string pafFilename = g_rootDir + "exported_" + outputPrefix + ".paf";
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
