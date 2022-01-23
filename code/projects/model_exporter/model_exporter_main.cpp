#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "core/time.hpp"
#include "glm/glm.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>

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


static void DisplayHelp()
{
    auto msg =
        "Usage: modelToPGModel PATH [texture_search_dir]"
        "\tIf PATH is a directory, all models found in it (not recursive) are converted into .pmodel files.\n"
        "\tIf PATH is a file, then only that one file is converted to a pmodel.\n"
        "\tAlso creates an asset file (.paf) containing all the model, material, and texture info\n";
    std::cout << msg << std::endl;
}


static std::string TrimWhiteSpace( const std::string& s )
{
    size_t start = s.find_first_not_of( " \t" );
    size_t end   = s.find_last_not_of( " \t" );
    return s.substr( start, end - start + 1 );
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


bool GetAssimpTexturePath( const aiMaterial* assimpMat, aiTextureType texType, std::string& pathToTex )
{
    aiString path;
    namespace fs = std::filesystem;
    if ( assimpMat->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
    {
        std::string name = TrimWhiteSpace( path.data );
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
        LOG_ERR( "Could not get texture of type: '%s'", TextureTypeToString( texType ) );
        return false;
    }

    return true;
}


static bool OutputMaterial( const aiMaterial* assimpMat, const aiScene* scene, const std::string& modelName, std::string& outputJSON )
{
    std::string name;
    glm::vec3 albedo = glm::vec3( 0 );
    std::string albedoMapName;

    aiString assimpMatName;
    aiColor3D color;

    assimpMat->Get( AI_MATKEY_NAME, assimpMatName );
    name = assimpMatName.C_Str();

    color = aiColor3D( 0.f, 0.f, 0.f );
    assimpMat->Get( AI_MATKEY_COLOR_DIFFUSE, color );
    albedo = { color.r, color.g, color.b };

    if ( assimpMat->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
    {
        std::string imagePath;
        if ( assimpMat->GetTextureCount( aiTextureType_DIFFUSE ) > 1 )
        {
            LOG_WARN( "Material '%s' has more than 1 diffuse texture", name.c_str() );
        }
        else if ( !GetAssimpTexturePath( assimpMat, aiTextureType_DIFFUSE, imagePath ) )
        {
            LOG_WARN( "Material '%s': can't find diffuse texture, using filename '%s'", name.c_str(), imagePath.c_str() );
        }
        if ( !imagePath.empty() )
        {
            albedoMapName = GetFilenameStem( imagePath );
            outputJSON += "\t{ \"Image\": {\n";
            outputJSON += "\t\t\"name\": \"" + albedoMapName + "\",\n";
            outputJSON += "\t\t\"filename\": \"" + imagePath + "\",\n";
            outputJSON += "\t\t\"semantic\": \"DIFFUSE\"\n";
            outputJSON += "\t} },\n";
        }
    }

    outputJSON += "\t{ \"Material\": {\n";
    outputJSON += "\t\t\"name\": \"" + name + "\",\n";
    outputJSON += "\t\t\"albedo\": [ " + std::to_string( albedo.r ) + ", " + std::to_string( albedo.g ) + ", " + std::to_string( albedo.b ) + " ]";
    if ( albedoMapName != "" )
    {
        outputJSON += ",\n\t\t\"albedoMap\": \"" + albedoMapName + "\"";
    }
    outputJSON += "\n\t} },\n";

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
    std::vector<uint32_t> indices;
    uint32_t numVertices = 0;
    uint32_t numIndices = 0;
    uint32_t numVerticesWithUVs = 0;
    std::string stem = GetFilenameStem( filename );

    std::vector<std::string> materialNames;
    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        std::string matName = scene->mMaterials[i]->GetName().C_Str();
        if ( matName.empty() )
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
                const glm::vec3& n = normals[vIdx];
                t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                if ( glm::any( glm::isnan( t ) ) )
                {
                    ++nanTangents;
                    t = glm::vec3( 0 );
                }

                tangents.emplace_back( t );
            }
            else if ( anyMeshHasUVs )
            {
                uvs.emplace_back( 0, 0 );
                tangents.emplace_back( 0, 0, 0 );
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

    outFile << "Positions: " << vertices.size() << "\n";
    for ( size_t i = 0; i < vertices.size(); ++i ) outFile << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";

    outFile << "Normals: " << normals.size() << "\n";
    for ( size_t i = 0; i < normals.size(); ++i ) outFile << normals[i].x << " " << normals[i].y << " " << normals[i].z << "\n";

    outFile << "TexCoords: " << uvs.size() << "\n";
    for ( size_t i = 0; i < uvs.size(); ++i ) outFile << uvs[i].x << " " << uvs[i].y << "\n";

    outFile << "Tangents: " << tangents.size() << "\n";
    for ( size_t i = 0; i < tangents.size(); ++i ) outFile << tangents[i].x << " " << tangents[i].y << " " << tangents[i].z << "\n";

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
    if ( IsDirectory( argv[1] ) )
    {
        directory = argv[1];
        g_textureSearchDir = argc > 2 ? argv[2] : directory;

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
        filesToProcess.push_back( argv[1] );
    }
    else
    {
        LOG_ERR( "Path '%s' does not exist!", argv[1] );
        return 0;
    }

    auto startTime = PG::Time::GetTimePoint();
    size_t modelsConverted = 0;
    std::string outputJSON = "[\n";
    outputJSON.reserve( 1024 * 1024 );
    #pragma omp parallel for
    for ( int i = 0; i < static_cast<int>( filesToProcess.size() ); ++i )
    {
        std::string json;
        json.reserve( 2 * 1024 );
        modelsConverted += ConvertModel( filesToProcess[i], json );
        outputJSON += json;
    }

    LOG( "Models converted: %u in %.2f seconds", modelsConverted, PG::Time::GetDuration( startTime ) / 1000.0f );
    LOG( "Errors: %zu, Warnings: %u", filesToProcess.size() - modelsConverted, g_warnings.load() );
    if ( modelsConverted > 0 )
    {
        outputJSON[outputJSON.length() - 2] = '\n';
        outputJSON[outputJSON.length() - 1] = ']';
        std::string pafBaseName = GetDirectoryStem( directory );
        if ( pafBaseName.empty() || pafBaseName[pafBaseName.length() - 1] == '.' )
        {
            pafBaseName = "default";
        }
        std::string pafFilename = directory + pafBaseName + ".paf";
        LOG( "Saving asset file %s", pafFilename.c_str() );
        std::ofstream out( pafFilename );
        out << outputJSON;
    }

    return 0;
}