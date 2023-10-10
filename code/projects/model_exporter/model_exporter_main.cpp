#include "model_exporter_common.hpp"
#include "model_exporter_materials.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/types/pmodel_versions.hpp"
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

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uvs[PMODEL_MAX_UVS_PER_VERT];
    vec4 colors[PMODEL_MAX_COLORS_PER_VERT];
    float boneWeights[PMODEL_MAX_BONE_WEIGHTS_PER_VERT];
    uint32_t boneIndices[PMODEL_MAX_BONE_WEIGHTS_PER_VERT];
    uint8_t numBones = 0;

    bool AddBone( uint32_t boneIdx, float weight )
    {
        if ( numBones < PMODEL_MAX_BONE_WEIGHTS_PER_VERT )
        {
            boneWeights[numBones] = weight;
            boneIndices[numBones] = boneIdx;
            ++numBones;
            return true;
        }

        float minWeight = boneWeights[0];
        uint32_t minIdx = 0;
        for ( uint32_t slot = 1; slot < numBones; ++slot )
        {
            if ( boneWeights[slot] < minWeight )
            {
                minWeight = boneWeights[slot];
                minIdx = slot;
            }
        }

        return false;
    }
};

struct Mesh
{
    std::string name;
    uint32_t matIndex;
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
    uint32_t numUVChannels;
    uint32_t numColorChannels;
    bool hasTangents;
    bool hasBoneWeights;
};


static void ProcessVertices( const aiMesh* paiMesh, Mesh& pgMesh )
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

    uint32_t numVerts = paiMesh->mNumVertices;
    pgMesh.vertices.resize( numVerts );
    for ( uint32_t vIdx = 0; vIdx < numVerts; ++vIdx )
    {
        Vertex& v = pgMesh.vertices[vIdx];
        v.pos = AiToPG( paiMesh->mVertices[vIdx] );
        v.normal = AiToPG( paiMesh->mNormals[vIdx] );
        PG_ASSERT( !any( isnan( v.pos ) ) );
        PG_ASSERT( !any( isnan( v.normal ) ) );
        v.numBones = 0;

        if ( pgMesh.hasTangents )
        {
            v.tangent = AiToPG( paiMesh->mTangents[vIdx] );
            v.bitangent = AiToPG( paiMesh->mBitangents[vIdx] );
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

        Vertex& v = pgMesh.vertices[vIdx];
        float weightTotal = 0;
        for ( uint8_t slot = 0; slot < v.numBones; ++slot )
            weightTotal += v.boneWeights[slot];

        for ( uint8_t slot = 0; slot < v.numBones; ++slot )
            v.boneWeights[slot] /= weightTotal;
    }
}


static std::string FloatToString( float x )
{
    if ( x == 0 )
        return "0";
    
    int truncated = (int)x;
    if ( x == (float)truncated )
        return std::to_string( truncated );

    char buffer[64];
    int len = sprintf( buffer, "%.6f", x );
    for ( int i = len - 1; i > 0; --i )
    {
        if ( buffer[i] == '.' )
            return std::string( buffer, i );

        if ( buffer[i] == '0' )
            continue;

        return std::string( buffer, i + 1 );
    }

    return std::string( buffer, len );
}

static std::string Vec2ToString( const vec2& v )
{
    return FloatToString( v.x ) + " " + FloatToString( v.y );
}

static std::string Vec3ToString( const vec3& v )
{
    return FloatToString( v.x ) + " " + FloatToString( v.y ) + " " + FloatToString( v.z );
}

static std::string Vec4ToString( const vec4& v )
{
    return FloatToString( v.x ) + " " + FloatToString( v.y ) + " " + FloatToString( v.z ) + " " + FloatToString( v.w );
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

    std::vector<Mesh> meshes( scene->mNumMeshes );
    std::string stem = GetFilenameStem( filename );
    for ( uint32_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx )
    {
        const aiMesh* paiMesh = scene->mMeshes[meshIdx];
        Mesh& pgMesh = meshes[meshIdx];
        pgMesh.name = paiMesh->mName.C_Str();
        if ( pgMesh.name.empty() )
        {
            pgMesh.name = stem + "_mesh" + std::to_string( meshIdx );
            LOG_WARN( "Mesh %u in file %s does not have a name. Assigning name %s", meshIdx, filename.c_str(), pgMesh.name.c_str() );
        }
        pgMesh.matIndex = paiMesh->mMaterialIndex;
        PG_ASSERT( pgMesh.matIndex < materialNames.size() );
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
            PG_ASSERT( face.mNumIndices == 3 );
            pgMesh.indices[faceIdx * 3 + 1] = face.mIndices[0];
            pgMesh.indices[faceIdx * 3 + 2] = face.mIndices[1];
            pgMesh.indices[faceIdx * 3 + 2] = face.mIndices[2];
        }
    }

    size_t totalVerts = 0;
    size_t totalTris = 0;
    for ( const Mesh& mesh : meshes )
    {
        totalVerts += mesh.vertices.size();
        totalTris += mesh.indices.size() / 3;
    }
    
    LOG( "Model %s\n\tMeshes: %u, Materials: %u, Triangles: %u\n\tVertices: %u",
        filename.c_str(), meshes.size(), materialNames.size(), totalTris, totalVerts );

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
        outFile << materialNames[i] << "\n";
    }
    outFile << "\n";

    const char* uvPrefixes[PMODEL_MAX_UVS_PER_VERT] = { "uv0 ", "uv1 ", "uv2 ", "uv3 ", "uv4 ", "uv5 ", "uv6 " ,"uv7 " };
    const char* colorPrefixes[PMODEL_MAX_COLORS_PER_VERT] = { "c0 ", "c1 ", "c2 ", "c3 ", "c4 ", "c5 ", "c6 " ,"c7 " };
    const char* bonePrefixes[PMODEL_MAX_BONE_WEIGHTS_PER_VERT] =
        { "b0 ", "b1 ", "b2 ", "b3 ", "b4 ", "b5 ", "b6 " ,"b7 ", "b8 ", "b9 ", "b10 ", "b11 ", "b12 ", "b13 ", "b14 " ,"b15 " };

    size_t vertsWritten = 0;
    const size_t tenPercentVerts = totalVerts / 10;
    int lastPercent = 0;
    for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
    {
        const Mesh& m = meshes[meshIdx];
        outFile << "Mesh " << meshIdx << ": " << m.name << "\n";
        outFile << "Mat: " << materialNames[m.matIndex] << "\n";
        outFile << "NumUVs: " << m.numUVChannels << "\n";
        outFile << "NumColors: " << m.numColorChannels << "\n\n";

        for ( uint32_t vIdx = 0; vIdx < (uint32_t)m.vertices.size(); ++vIdx )
        {
            const Vertex& v = m.vertices[vIdx];
            outFile << "V " << vIdx << "\n";
            outFile << "p " << Vec3ToString( v.pos ) << "\n";
            outFile << "n " << Vec3ToString( v.normal ) << "\n";
            if ( m.hasTangents )
            {
                outFile << "t " << Vec3ToString( v.tangent ) << "\n";
                outFile << "b " << Vec3ToString( v.bitangent ) << "\n";
            }
            for ( uint32_t i = 0; i < m.numUVChannels; ++i )
                outFile << uvPrefixes[i] << Vec2ToString( v.uvs[i] ) << "\n";
            for ( uint32_t i = 0; i < m.numColorChannels; ++i )
                outFile << colorPrefixes[i] << Vec4ToString( v.colors[i] ) << "\n";

            for ( uint8_t i = 0; i < v.numBones; ++i )
            {
                outFile << bonePrefixes[i] << v.boneIndices[i] << FloatToString( v.boneWeights[i] ) << "\n";
            }

            outFile << "\n";
        }

        outFile << "Tris:\n";
        for ( size_t i = 0; i < m.indices.size(); i += 3 )
            outFile << m.indices[i+0] << " " << m.indices[i+1] << " " << m.indices[i+2] << "\n";

        vertsWritten += m.vertices.size();
        int currentPercent = static_cast<int>( vertsWritten / (double)totalVerts * 100 );
        if ( currentPercent - lastPercent >= 10 )
        {
            LOG( "%d%%...", currentPercent );
            lastPercent = currentPercent;
        }
    }

    std::string relPath = GetPathRelativeToAssetDir( outputModelFilename );
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
