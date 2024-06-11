#include "asset/asset_file_database.hpp"
#include "asset/pmodel.hpp"
#include "asset/types/textureset.hpp"
#include "core/time.hpp"
#include "getopt/getopt.h"
#include "model_exporter_common.hpp"
#include "model_exporter_materials.hpp"
#include "shared/filesystem.hpp"
#include "shared/hash.hpp"
#include "shared/logger.hpp"
#include "shared/math.hpp"
#include "shared/string.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace PG;

static_assert( PMODEL_MAX_UVS_PER_VERT == AI_MAX_NUMBER_OF_TEXTURECOORDS );
static_assert( PMODEL_MAX_COLORS_PER_VERT == AI_MAX_NUMBER_OF_COLOR_SETS );

std::atomic<u32> g_warnings;

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
    u32 uvSetRemap[AI_MAX_NUMBER_OF_TEXTURECOORDS] = { UINT_MAX };
    u32 colorSetRemap[AI_MAX_NUMBER_OF_COLOR_SETS] = { UINT_MAX };
    static_assert( AI_MAX_NUMBER_OF_TEXTURECOORDS == 8 );
    {
        u32 pgUVIdx    = 0;
        u32 pgColorIdx = 0;
        for ( u32 i = 0; i < 8; ++i )
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

    mat4 normalMatrix = Inverse( Transpose( localToWorldMat ) );

    u32 numZeroBefore = 0;
    u32 numZeroAfter  = 0;
    u32 numVerts      = paiMesh->mNumVertices;
    pgMesh.vertices.resize( numVerts );
    for ( u32 vIdx = 0; vIdx < numVerts; ++vIdx )
    {
        PModel::Vertex& v = pgMesh.vertices[vIdx];
        v.pos             = AiToPG( paiMesh->mVertices[vIdx] );
        v.pos             = vec3( localToWorldMat * vec4( v.pos, 1.0f ) );
        v.normal          = AiToPG( paiMesh->mNormals[vIdx] );
        if ( Length( v.normal ) == 0 )
            numZeroBefore++;
        v.normal = vec3( normalMatrix * vec4( v.normal, 0.0f ) );
        if ( Length( v.normal ) == 0 )
            numZeroAfter++;
        PG_ASSERT( !any( isnan( v.pos ) ) );
        PG_ASSERT( !any( isnan( v.normal ) ) );
        v.numBones = 0;

        if ( pgMesh.hasTangents )
        {
            v.tangent   = AiToPG( paiMesh->mTangents[vIdx] );
            v.tangent   = vec3( localToWorldMat * vec4( v.tangent, 0.0f ) );
            v.bitangent = AiToPG( paiMesh->mBitangents[vIdx] );
            v.bitangent = vec3( localToWorldMat * vec4( v.bitangent, 0.0f ) );
        }

        for ( u32 uvSetIdx = 0; uvSetIdx < pgMesh.numUVChannels; ++uvSetIdx )
        {
            u32 aiUVSetIdx  = uvSetRemap[uvSetIdx];
            vec3 uvw        = AiToPG( paiMesh->mTextureCoords[aiUVSetIdx][vIdx] );
            v.uvs[uvSetIdx] = vec2( uvw.x, uvw.y );
        }

        for ( u32 colorSetIdx = 0; colorSetIdx < pgMesh.numColorChannels; ++colorSetIdx )
        {
            u32 aiColorSetIdx     = colorSetRemap[colorSetIdx];
            v.colors[colorSetIdx] = AiToPG( paiMesh->mColors[aiColorSetIdx][vIdx] );
        }
    }

    if ( numZeroBefore || numZeroAfter )
        LOG( "Num normals with length zero: pre-transform %u, post %u", numZeroBefore, numZeroAfter );

    std::vector<u32> numBonesPerVertex( numVerts, 0 );
    for ( u32 aiBoneIdx = 0; aiBoneIdx < paiMesh->mNumBones; ++aiBoneIdx )
    {
        const aiBone& paiBone = *paiMesh->mBones[aiBoneIdx];
        for ( u32 weightIdx = 0; weightIdx < paiBone.mNumWeights; ++weightIdx )
        {
            const aiVertexWeight& w = paiBone.mWeights[weightIdx];
            PG_ASSERT( w.mVertexId < numVerts );
            numBonesPerVertex[w.mVertexId]++;
            pgMesh.vertices[w.mVertexId].AddBone( aiBoneIdx, w.mWeight );
        }
    }

    for ( u32 vIdx = 0; vIdx < numVerts; ++vIdx )
    {
        if ( numBonesPerVertex[vIdx] > PMODEL_MAX_BONE_WEIGHTS_PER_VERT )
        {
            LOG_WARN( "Vertex %u in mesh %s has %u bones weights in the source file, but Pmodels only support %u per vertex. Using the %u "
                      "highest weights instead",
                vIdx, pgMesh.name.c_str(), numBonesPerVertex[vIdx], PMODEL_MAX_BONE_WEIGHTS_PER_VERT, PMODEL_MAX_BONE_WEIGHTS_PER_VERT );
        }

        PModel::Vertex& v = pgMesh.vertices[vIdx];
        f32 weightTotal   = 0;
        for ( u8 slot = 0; slot < v.numBones; ++slot )
            weightTotal += v.boneWeights[slot];

        for ( u8 slot = 0; slot < v.numBones; ++slot )
            v.boneWeights[slot] /= weightTotal;
    }
}

void ParseNode( const std::string& filename, const std::vector<std::string>& materialNames, const aiScene* scene, const aiNode* node,
    const mat4& parentLocalToWorld, PModel& pmodel )
{
    std::string stem         = GetFilenameStem( filename );
    mat4 currentLocalToWorld = AiToGLMMat4( node->mTransformation ) * parentLocalToWorld;
    for ( u32 meshIdx = 0; meshIdx < node->mNumMeshes; ++meshIdx )
    {
        const aiMesh* paiMesh = scene->mMeshes[node->mMeshes[meshIdx]];
        PModel::Mesh& pgMesh  = pmodel.meshes.emplace_back();
        pgMesh.name           = StripWhitespace( paiMesh->mName.C_Str() );
        if ( pgMesh.name.empty() )
        {
            pgMesh.name = stem + "_mesh" + std::to_string( meshIdx );
            LOG_WARN( "Mesh %u in file %s does not have a name. Assigning name %s", meshIdx, filename.c_str(), pgMesh.name.c_str() );
        }
        pgMesh.materialName     = materialNames[paiMesh->mMaterialIndex];
        pgMesh.numColorChannels = paiMesh->GetNumColorChannels();
        pgMesh.numUVChannels    = paiMesh->GetNumUVChannels();
        pgMesh.hasTangents      = paiMesh->mTangents && paiMesh->mBitangents;
        pgMesh.hasBoneWeights   = paiMesh->HasBones();

        if ( paiMesh->mNumAnimMeshes )
        {
            LOG_WARN( "Mesh %u '%s' in file %s contains attachment/anim meshes. Currently not supported", meshIdx, filename.c_str(),
                pgMesh.name.c_str() );
        }

        for ( u32 uvSetIdx = 0; uvSetIdx < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++uvSetIdx )
        {
            if ( !paiMesh->HasTextureCoords( uvSetIdx ) || paiMesh->mNumUVComponents[uvSetIdx] == 2 )
                continue;

            if ( paiMesh->HasTextureCoordsName( uvSetIdx ) )
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u '%s' has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->GetTextureCoordsName( uvSetIdx )->C_Str(),
                    paiMesh->mNumUVComponents[uvSetIdx] );
            }
            else
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u (unnamed) has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->mNumUVComponents[uvSetIdx] );
            }
        }

        ProcessVertices( paiMesh, currentLocalToWorld, pgMesh );

        pgMesh.indices.resize( paiMesh->mNumFaces * 3 );
        for ( u32 faceIdx = 0; faceIdx < paiMesh->mNumFaces; ++faceIdx )
        {
            const aiFace& face              = paiMesh->mFaces[faceIdx];
            pgMesh.indices[faceIdx * 3 + 0] = face.mIndices[0];
            pgMesh.indices[faceIdx * 3 + 1] = face.mIndices[1];
            pgMesh.indices[faceIdx * 3 + 2] = face.mIndices[2];
        }
    }

    for ( u32 childIdx = 0; childIdx < node->mNumChildren; ++childIdx )
    {
        ParseNode( filename, materialNames, scene, node->mChildren[childIdx], currentLocalToWorld, pmodel );
    }
}

static bool ConvertModel( const std::string& filename, std::string& outputJSON )
{
    LOG( "Parsing file %s...", filename.c_str() );
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile( filename.c_str(), aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices |
                                                 aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
    if ( !scene )
    {
        LOG_ERR( "Error parsing model file '%s': '%s'", filename.c_str(), importer.GetErrorString() );
        return false;
    }

    std::vector<std::string> materialNames;
    bool isOBJ = GetFileExtension( filename ) == ".obj";
    for ( u32 i = 0; i < scene->mNumMaterials; ++i )
    {
        std::string matName = scene->mMaterials[i]->GetName().C_Str();
        if ( matName == "DefaultMaterial" || matName == "default" || ( isOBJ && matName == "None" ) )
            matName = "default";
        else if ( matName == "" )
            matName = "material_" + std::to_string( i );

        materialNames.push_back( matName );
    }

    std::string modelName = GetFilenameStem( filename );

    MaterialContext matContext;
    matContext.scene     = scene;
    matContext.file      = filename;
    matContext.modelName = modelName;
    for ( u32 i = 0; i < scene->mNumMaterials; ++i )
    {
        if ( materialNames[i] != "default" )
        {
            matContext.assimpMat    = scene->mMaterials[i];
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
    for ( u32 meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx )
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

        for ( u32 uvSetIdx = 0; uvSetIdx < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++uvSetIdx )
        {
            if ( !paiMesh->HasTextureCoords( uvSetIdx ) || paiMesh->mNumUVComponents[uvSetIdx] == 2 )
                continue;

            if ( paiMesh->HasTextureCoordsName( uvSetIdx ) )
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u '%s' has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->GetTextureCoordsName( uvSetIdx )->C_Str(),
    paiMesh->mNumUVComponents[uvSetIdx] );
            }
            else
            {
                LOG_WARN( "Mesh %u '%s' in file %s: uv set %u (unnamed) has %u channels. Expecting 2, will ignore this uv set", meshIdx,
                    filename.c_str(), pgMesh.name.c_str(), uvSetIdx, paiMesh->mNumUVComponents[uvSetIdx] );
            }
        }

        ProcessVertices( paiMesh, pgMesh );

        pgMesh.indices.resize( paiMesh->mNumFaces * 3 );
        for ( u32 faceIdx = 0; faceIdx < paiMesh->mNumFaces; ++faceIdx )
        {
            const aiFace& face = paiMesh->mFaces[faceIdx];
            pgMesh.indices[faceIdx * 3 + 0] = face.mIndices[0];
            pgMesh.indices[faceIdx * 3 + 1] = face.mIndices[1];
            pgMesh.indices[faceIdx * 3 + 2] = face.mIndices[2];
        }
    }
    */

    size_t totalVerts = 0;
    size_t totalTris  = 0;
    for ( const PModel::Mesh& mesh : pmodel.meshes )
    {
        totalVerts += mesh.vertices.size();
        totalTris += mesh.indices.size() / 3;
    }

    LOG( "Model %s\n\tMeshes: %u, Materials: %u, Triangles: %u\n\tVertices: %u", filename.c_str(), pmodel.meshes.size(),
        materialNames.size(), totalTris, totalVerts );

    std::string outputModelFilename = GetFilenameMinusExtension( filename ) + ".pmodelb";
    if ( !pmodel.Save( outputModelFilename, g_options.floatPrecision, true ) )
        return false;

    std::string relPath = GetRelativePathToDir( outputModelFilename, g_options.rootDir );
    outputJSON += "\t{ \"Model\": { \"name\": \"" + modelName + "\", \"filename\": \"" + relPath + "\" } },\n";

    LOG( "Done processing file %s", filename.c_str() );

    return true;
}

static void DisplayHelp()
{
    auto msg = "Usage: ModelExporter [options] PATH\n"
               "If PATH is a directory, all models found in it (not recursive) are converted into .pmodelb files.\n"
               "\tIf PATH is an asset file, then only that one file is converted to a pmodel.\n"
               "\tAlso creates an asset file (.paf) containing all the model, material, and texture info\n"
               "\tIf PATH is .pmodelb file, then it will save out the corresponding .pmodelt file.\n"
               "\tIf PATH is .pmodelt file, then it will save out the corresponding .pmodelb file.\n"
               "Options\n"
               "  --floatPrecision [1-9] Specify how many float sig figs to write out. Default is 6 (max is 9)\n"
               "  --help                 Print this message and exit\n"
               "  --ignoreCollisions     Skip renaming any assets that have the same name as an existing asset in the database\n"
               "  --texDir [path]        Specify a different directory to use to search for the textures\n";
    std::cout << msg << std::endl;
}

static bool ParseCommandLineArgs( int argc, char** argv, std::string& path )
{
    static struct option long_options[] = {
        {"floatPrecision",   required_argument, 0, 'f'},
        {"ignoreCollisions", no_argument,       0, 'i'},
        {"help",             no_argument,       0, 'h'},
        {"texDir",           required_argument, 0, 't'},
        {0,                  0,                 0, 0  }
    };

    i32 option_index = 0;
    i32 c            = -1;
    while ( ( c = getopt_long( argc, argv, "f:iht:", long_options, &option_index ) ) != -1 )
    {
        switch ( c )
        {
        case 'f': g_options.floatPrecision = std::stoul( optarg ); break;
        case 'h': DisplayHelp(); return false;
        case 'i': g_options.ignoreNameCollisions = true; break;
        case 't': g_textureSearchDir = optarg; break;
        default: LOG_ERR( "Invalid option, try 'ModelExporter --help' for more information" ); return false;
        }
    }

    if ( optind >= argc )
    {
        DisplayHelp();
        return false;
    }

    path = argv[optind];

    return true;
}

static void ConvertPModelFiletype( const std::string& filename )
{
    bool isText = GetFileExtension( filename ) == ".pmodelt";
    PModel pmodel;
    if ( !pmodel.Load( filename ) )
    {
        LOG_ERR( "Failed to load pmodel file '%s'", filename.c_str() );
        return;
    }

    std::string newFilename = GetFilenameMinusExtension( filename );
    newFilename += isText ? ".pmodelb" : ".pmodelt";
    pmodel.Save( newFilename, g_options.floatPrecision );
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

    std::string path;
    if ( !ParseCommandLineArgs( argc, argv, path ) )
    {
        DisplayHelp();
        return 0;
    }

    if ( GetFileExtension( path ) == ".pmodelt" || GetFileExtension( path ) == ".pmodelb" )
    {
        ConvertPModelFiletype( path );
        return 0;
    }

    g_warnings                                      = 0;
    std::unordered_set<std::string> modelExtensions = { ".obj", ".fbx", ".ply", ".gltf", ".stl" };
    std::vector<std::string> filesToProcess;
    std::string outputPrefix;
    if ( IsDirectory( path ) )
    {
        g_options.rootDir = path;
        if ( g_textureSearchDir.empty() )
            g_textureSearchDir = g_options.rootDir;
        outputPrefix = GetDirectoryStem( g_options.rootDir );

        namespace fs = std::filesystem;
        for ( const auto& entry : fs::directory_iterator( g_options.rootDir ) )
        {
            std::string ext = GetFileExtension( entry.path().string() );
            if ( entry.is_regular_file() && modelExtensions.find( ext ) != modelExtensions.end() )
            {
                filesToProcess.push_back( entry.path().string() );
            }
        }
    }
    else if ( IsFile( path ) )
    {
        g_options.rootDir = GetParentPath( path );
        if ( g_textureSearchDir.empty() )
            g_textureSearchDir = g_options.rootDir;
        outputPrefix = GetFilenameStem( path );
        filesToProcess.push_back( path );
    }
    else
    {
        LOG_ERR( "Path '%s' does not exist!", path.c_str() );
        return 0;
    }

    PG_ASSERT( g_options.rootDir.length() );
    AddTrailingSlashIfMissing( g_options.rootDir );

    // if we are re-exporting, delete before initializing the database, so that it doesn't affect the unique name generation
    std::string pafFilename = g_options.rootDir + "exported_" + outputPrefix + ".paf";
    if ( PathExists( pafFilename ) )
    {
        DeleteFile( pafFilename );
    }
    AssetDatabase::Init();

    auto startTime         = Time::GetTimePoint();
    size_t modelsConverted = 0;
    std::string outputJSON = "[\n";
    outputJSON.reserve( 1024 * 1024 );
    // Cant run in parallel, since writing the output
    // #pragma omp parallel for
    for ( i32 i = 0; i < static_cast<i32>( filesToProcess.size() ); ++i )
    {
        std::string json;
        json.reserve( 2 * 1024 );
        modelsConverted += ConvertModel( filesToProcess[i], json );
        outputJSON += json;
    }

    LOG( "Models converted: %u in %.2f seconds", modelsConverted, Time::GetTimeSince( startTime ) / 1000.0f );
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
