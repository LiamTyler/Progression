#include "resource/model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "configuration.hpp"
#include "intersection_tests.hpp"
#include "resource/resource_manager.hpp"
#include "utils/logger.hpp"
#include "utils/time.hpp"
#include <algorithm>
#include <filesystem>
#include <stack>

namespace PT
{
    static std::string TrimWhiteSpace( const std::string& s )
    {
        size_t start = s.find_first_not_of( " \t" );
        size_t end   = s.find_last_not_of( " \t" );
        return s.substr( start, end - start + 1 );
    }

    static std::shared_ptr< Texture > LoadAssimpTexture( const aiMaterial* pMaterial, aiTextureType texType )
    {
        namespace fs = std::filesystem;
        aiString path;
        if ( pMaterial->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
        {
            std::string name = TrimWhiteSpace( path.data );
            if ( ResourceManager::GetTexture( name ) )
            {
                return ResourceManager::GetTexture( name );
            }

            std::string fullPath;
            // search for texture starting with
            if ( fs::exists( RESOURCE_DIR + name ) )
            {
                fullPath = RESOURCE_DIR + name;
            }
            else
            {
                std::string basename = fs::path( name ).filename().string();
                for( auto itEntry = fs::recursive_directory_iterator( RESOURCE_DIR ); itEntry != fs::recursive_directory_iterator(); ++itEntry )
                {
                    std::string itFile = itEntry->path().filename().string();
                    if ( basename == itEntry->path().filename().string() )
                    {
                        fullPath = fs::absolute( itEntry->path() ).string();
                        break;
                    }
                }
            }
                    
            if ( fullPath != "" )
            {
                TextureCreateInfo info;
                info.name     = std::filesystem::path( fullPath ).stem().string();;
                info.filename = fullPath;
                auto ret = std::make_shared< Texture >();
                if ( !ret->Load( info ) )
                {
                    LOG_ERR( "Failed to load texture '", name, "' with default settings" );
                    return nullptr;
                }
                ret->name = name;
                return ret;
            }
            else
            {
                LOG_ERR( "Could not find image file '", name,"'" );
            }
        }
        else
        {
            LOG_ERR( "Could not get texture of type: ", texType );
        }

        return nullptr;
    }

    static bool ParseMaterials( const std::string& filename, Model* model, const aiScene* scene )
    {
        std::vector< std::shared_ptr< Material > > materials( scene->mNumMaterials );
        for ( uint32_t mtlIdx = 0; mtlIdx < scene->mNumMaterials; ++mtlIdx )
        {
            const aiMaterial* pMaterial = scene->mMaterials[mtlIdx];
            auto newMat = std::make_shared< Material >();
            materials[mtlIdx] = newMat;

            aiString name;
            aiColor3D color;
            pMaterial->Get( AI_MATKEY_NAME, name );
            newMat->name = name.C_Str();

            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_DIFFUSE, color );
            newMat->albedo = { color.r, color.g, color.b };

            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_SPECULAR, color );
            newMat->Ks = { color.r, color.g, color.b };
            
            float tmp;
            pMaterial->Get( AI_MATKEY_SHININESS, tmp );
            newMat->Ns = tmp;

            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_EMISSIVE, color );
            newMat->Ke = { color.r, color.g, color.b };

            if ( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
            {
                assert( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) == 1 );
                newMat->albedoTexture = LoadAssimpTexture( pMaterial, aiTextureType_DIFFUSE );
                if ( !newMat->albedoTexture )
                {
                    return false;
                }
            }
        }

        for ( size_t i = 0 ; i < model->meshes.size(); i++ )
        {
            model->meshes[i].material = materials[scene->mMeshes[i]->mMaterialIndex];
        }

        return true;
    }

    void Mesh::RecalculateNormals()
    {
        normals.clear();
        normals.resize( vertices.size() );
        for ( auto& normal : normals )
        {
            normal = glm::vec3( 0 );
        }

        for ( size_t i = 0; i < indices.size(); i += 3 )
        {
            glm::vec3 v1 = vertices[indices[i + 0]];
            glm::vec3 v2 = vertices[indices[i + 1]];
            glm::vec3 v3 = vertices[indices[i + 2]];
            glm::vec3 n = glm::normalize( glm::cross( v2 - v1, v3 - v1 ) );
            normals[indices[i + 0]] += n;
            normals[indices[i + 1]] += n;
            normals[indices[i + 2]] += n;
            
        }

        for ( auto& normal : normals )
        {
            normal = glm::normalize( normal );
        }
    }

    bool Model::Load( const ModelCreateInfo& createInfo )
    {
        name = createInfo.name;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile( createInfo.filename.c_str(),
            aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace | aiProcess_RemoveRedundantMaterials );
        if ( !scene )
        {
            LOG_ERR( "Error parsing model file '", createInfo.filename.c_str(), "': ", importer.GetErrorString() );
            return false;
        }

        meshes.resize( scene->mNumMeshes );
        for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
        {
            const aiMesh* paiMesh = scene->mMeshes[meshIdx];
            const aiVector3D Zero3D( 0.0f, 0.0f, 0.0f );
            Mesh& mesh = meshes[meshIdx];
            mesh.name  = paiMesh->mName.C_Str();
            mesh.vertices.reserve( paiMesh->mNumVertices );
            mesh.normals.reserve( paiMesh->mNumVertices );
            mesh.uvs.reserve( paiMesh->mNumVertices );
            mesh.tangents.reserve( paiMesh->mNumVertices );
            mesh.indices.reserve( paiMesh->mNumFaces * 3 );

            for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
            {
                const aiVector3D* pPos    = &paiMesh->mVertices[vIdx];
                glm::vec3 pos( pPos->x, pPos->y, pPos->z );
                mesh.vertices.emplace_back( pos );

                if ( paiMesh->HasNormals() )
                {
                    const aiVector3D* pNormal = &paiMesh->mNormals[vIdx];
                    mesh.normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );
                }

                if ( paiMesh->HasTextureCoords( 0 ) )
                {
                    const aiVector3D* pTexCoord = &paiMesh->mTextureCoords[0][vIdx];
                    mesh.uvs.emplace_back( pTexCoord->x, pTexCoord->y );
                }
                else
                {
                    mesh.uvs.emplace_back( 0, 0 );
                }
                if ( paiMesh->HasTangentsAndBitangents() )
                {
                    const aiVector3D* pTangent = &paiMesh->mTangents[vIdx];
                    glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                    const glm::vec3& n = mesh.normals[vIdx];
                    t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                    mesh.tangents.emplace_back( t );
                }
            }

            for ( size_t iIdx = 0; iIdx < paiMesh->mNumFaces; ++iIdx )
            {
                const aiFace& face = paiMesh->mFaces[iIdx];
                mesh.indices.push_back( face.mIndices[0] );
                mesh.indices.push_back( face.mIndices[1] );
                mesh.indices.push_back( face.mIndices[2] );
            }

            if ( createInfo.recalculateNormals )
            {
                mesh.RecalculateNormals();
            }

            if ( mesh.tangents.size() == 0 )
            {
                // std::cout << "WARNING: model '" << name << "', mesh '" << mesh.name << "' had no uvs, so assimp could not generate tangents. Generating arbitrary tangent basis." << std::endl;
                mesh.tangents.resize( mesh.vertices.size(), glm::vec3( 0 ) );
                for ( size_t i = 0; i < mesh.indices.size(); i += 3 )
                {
                    uint32_t i0  = mesh.indices[i + 0];
                    uint32_t i1  = mesh.indices[i + 1];
                    glm::vec3 v0 = mesh.vertices[i0];
                    glm::vec3 v1 = mesh.vertices[i1];
                    glm::vec3 n0 = mesh.normals[i0];
                    glm::vec3 n1 = mesh.normals[i1];
                    glm::vec3 t  = glm::normalize( v1 - v0 );
                    glm::vec3 t0 = glm::normalize( t - n0 * glm::dot( n0, t ) );
                    glm::vec3 t1 = glm::normalize( t - n1 * glm::dot( n1, t ) );
                    if ( mesh.tangents[i0] == glm::vec3( 0 ) )
                    {
                        mesh.tangents[i0] = t0;
                    }
                    if ( mesh.tangents[i1] == glm::vec3( 0 ) )
                    {
                        mesh.tangents[i1] = t1;
                    }
                }
            }
        }

        if ( !ParseMaterials( createInfo.filename, this, scene ) )
        {
            LOG_ERR( "Could not load the model's materials" );
            return false;
        }

        return true;
    }

    void Model::RecalculateNormals()
    {
        for ( auto& mesh : meshes )
        {
            mesh.RecalculateNormals();
        }
    }

    MeshInstance::MeshInstance( const Mesh& localMesh, const Transform& _localToWorld, std::shared_ptr< Material > newMaterial ) :
        localToWorld( _localToWorld ),
        worldToLocal( _localToWorld.Inverse() )
    {
        size_t numVertices = localMesh.vertices.size();
        assert( localMesh.normals.size() == numVertices && localMesh.tangents.size() == numVertices );
        data.vertices.resize( numVertices );
        for ( size_t i = 0; i < numVertices; ++i )
            data.vertices[i] = localToWorld.TransformPoint( localMesh.vertices[i] );
        data.normals.resize( numVertices );
        for ( size_t i = 0; i < numVertices; ++i )
            data.normals[i] = glm::normalize( worldToLocal.Transpose().TransformVector( localMesh.normals[i] ) );
        data.tangents.resize( numVertices );
        for ( size_t i = 0; i < numVertices; ++i )
            data.tangents[i] = localToWorld.TransformVector( localMesh.tangents[i] );
        data.uvs      = localMesh.uvs;
        data.indices  = localMesh.indices;
        data.material = newMaterial ? newMaterial : localMesh.material;
    }

    void MeshInstance::EmitTrianglesAndLights( std::vector< std::shared_ptr< Shape > >& shapes,
        std::vector< Light* >& lights, std::shared_ptr< MeshInstance > meshPtr ) const
    {
        shapes.reserve( shapes.size() + data.indices.size() / 3 );
        if ( data.material->Ke != glm::vec3( 0 ) )
        {
            lights.reserve( lights.size() + data.indices.size() / 3 );
        }

        for ( size_t face = 0; face < data.indices.size() / 3; ++face )
        {
            auto tri           = std::make_shared< Triangle >();
            tri->mesh          = meshPtr;
            tri->i0            = data.indices[3*face + 0];
            tri->i1            = data.indices[3*face + 1];
            tri->i2            = data.indices[3*face + 2];
            shapes.push_back( tri );
            if ( data.material->Ke != glm::vec3( 0 ) )
            {
                auto areaLight   = new AreaLight;
                areaLight->Lemit = data.material->Ke;
                areaLight->shape = tri;
                lights.push_back( areaLight );
            }
        }
    }

} // namespace PT
