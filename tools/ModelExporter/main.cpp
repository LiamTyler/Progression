#include "core/assert.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "glm/glm.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>

std::string g_textureSearchDir;

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
        "Usage: modelToPGModel MODEL_DIR"
        "\tConverts all models found in the MODEL_DIR (not recursive) into .pmodel files\n"
        "\tAlso creates an asset file (.paf) containing all the model, material, and texture info\n";

    std::cout << msg << std::endl;
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
    }
    else
    {
        LOG_ERR( "Could not get texture of type: '%s'", TextureTypeToString( texType ) );
        return false;
    }

    return true;
}


static bool OutputMaterial( const aiMaterial* assimpMat, const aiScene* scene, std::string& outputJSON )
{
    std::string name;
    glm::vec3 albedo = glm::vec3( 0 );
    std::string imagePath;
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
        if ( assimpMat->GetTextureCount( aiTextureType_DIFFUSE ) > 1 )
        {
            LOG_WARN( "Material '%s' has more than 1 diffuse texture", name.c_str() );
        }
        else if ( !GetAssimpTexturePath( assimpMat, aiTextureType_DIFFUSE, imagePath ) )
        {
            LOG_WARN( "Material '%s': can't find diffuse texture, using filename '%s'", imagePath.c_str() );
            albedoMapName = imagePath;
        }
        else
        {
            albedoMapName = imagePath;
        }
        //albedoMapName = GetRelativePathToDir( GetFilenameMinusExtension( imagePath ), g_parentToOutputDir );
        //ImageInfo info;
        //info.name     = albedoMapName;
        //info.filename = GetRelativePathToDir( imagePath, g_parentToOutputDir );
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

    std::vector<std::string> materialNames;
    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        std::string matName = scene->mMaterials[i]->GetName().C_Str();
        materialNames.push_back( matName );
    }

    for ( size_t i = 0 ; i < meshes.size(); i++ )
    {
        meshes[i].name = scene->mMeshes[i]->mName.C_Str();
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
    
        for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
        {
            const aiVector3D* pPos    = &paiMesh->mVertices[vIdx];
            const aiVector3D* pNormal = &paiMesh->mNormals[vIdx];
            vertices.emplace_back( pPos->x, pPos->y, pPos->z );
            normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );
    
            if ( paiMesh->HasTextureCoords( 0 ) )
            {
                const aiVector3D* pTexCoord = &paiMesh->mTextureCoords[0][vIdx];
                uvs.emplace_back( pTexCoord->x, pTexCoord->y );
    
                const aiVector3D* pTangent = &paiMesh->mTangents[vIdx];
                glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                const glm::vec3& n = normals[vIdx];
                t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                tangents.emplace_back( t );
            }
            else if ( anyMeshHasUVs )
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
    
    LOG( "Model %s", filename.c_str() );
    LOG( "Meshes: %u", meshes.size() );
    LOG( "Materials: %u", materialNames.size() );
    LOG( "Vertices: %u", vertices.size() );
    LOG( "Normals: %u", normals.size() );
    LOG( "uvs: %u", uvs.size() );
    LOG( "tangents: %u", tangents.size() );
    LOG( "triangles: %u", indices.size() / 3 );

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

    for ( unsigned int i = 0; i < scene->mNumMaterials; ++i )
    {
        OutputMaterial( scene->mMaterials[i], scene, outputJSON );
    }

    std::string modelName = GetFilenameMinusExtension( filename );
    outputJSON += "\t{ \"Model\": { \"name\": \"" + modelName + "\", \"filename\": \"" + outputModelFilename + "\" } }\n";

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

    std::string directory = argv[1];
    g_textureSearchDir = argc > 2 ? argv[2] : directory;

    std::unordered_set<std::string> modelExtensions = { ".obj", ".fbx", ".ply", ".gltf", ".stl" };
    size_t modelsConverted = 0;
    std::string outputJSON = "[\n";
    namespace fs = std::filesystem;
    try
    {
        for ( const auto& entry : fs::directory_iterator( directory ) )
        {
            std::string ext = GetFileExtension( entry.path().string() );
            if ( entry.is_regular_file() && modelExtensions.find( ext ) != modelExtensions.end() )
            {
                modelsConverted += ConvertModel( entry.path().string(), outputJSON );
            }
        }
    }
    catch ( std::filesystem::filesystem_error )
    {
        LOG_ERR( "Cannot find directory %s\n", directory.c_str() );
        return 0;
    }

    LOG( "Models converted: %u", modelsConverted );
    if ( modelsConverted > 0 )
    {
        std::string pafBaseName = GetDirectoryStem( directory );
        if ( pafBaseName.empty() || pafBaseName[pafBaseName.length() - 1] == '.' )
        {
            pafBaseName = "default";
        }
        std::string pafFilename = directory + pafBaseName + ".paf";
        LOG( "Saving asset file %s", pafFilename.c_str() );
        std::ofstream out( pafFilename );
        out << outputJSON << "]";
    }

    //std::string imgTypes[] =
    //{
    //    "TYPE_1D",
    //    "TYPE_1D_ARRAY",
    //    "TYPE_2D",
    //    "TYPE_2D_ARRAY",
    //    "TYPE_CUBEMAP",
    //    "TYPE_CUBEMAP_ARRAY",
    //    "TYPE_3D"
    //};
    //
    //std::string imgSemantics[] =
    //{
    //    "DIFFUSE",
    //    "NORMAL"
    //};
    //
    //std::string assetListJson = "{ \"AssetList\":\n[";
    //for ( size_t i = 0; i < imageInfos.size(); ++i )
    //{
    //    const auto& info = imageInfos[i];
    //    assetListJson += "\n\t{ \"Image\": { ";
    //    assetListJson += "\"name\": \"" + info.name + "\", ";
    //    assetListJson += "\"filename\": \"" + info.filename + "\", ";
    //    assetListJson += "\"semantic\": \"" + info.semantic + "\", ";
    //    assetListJson += "\"imageType\": \"" + info.type + "\" } },";
    //}

    return 0;
}