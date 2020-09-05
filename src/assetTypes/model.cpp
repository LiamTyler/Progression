#include "assetTypes/model.hpp"
#include "assert.hpp"
#include "assetTypes/material.hpp"
#include "asset_manager.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "glm/geometric.hpp"


namespace Progression
{


void Model::RecalculateNormals()
{
    PG_ASSERT( vertexPositions.size() == otherVertexData.size() );
    std::vector< glm::vec3 > newNormals;
    newNormals.resize( vertexPositions.size(), glm::vec3( 0 ) );
    for ( size_t i = 0; i < indices.size(); i += 3 )
    {
        const auto i0 = indices[i + 0];
        const auto i1 = indices[i + 1];
        const auto i2 = indices[i + 2];
        glm::vec3 e01 = vertexPositions[i0] - vertexPositions[i0];
        glm::vec3 e02 = vertexPositions[i2] - vertexPositions[i0];
        glm::vec3 n = glm::normalize( glm::cross( e01, e02 ) );
        newNormals[i0] += n;
        newNormals[i1] += n;
        newNormals[i2] += n;
    }

    for ( size_t i = 0; i < vertexPositions.size(); ++i )
    {
        otherVertexData[i].normal = glm::normalize( newNormals[i] );
    }
}


bool Model_Load_NoResolveMaterials( Model* model, const ModelCreateInfo& createInfo, std::vector< std::string >& matNames )
{
    static_assert( sizeof( Model ) == 8 + sizeof( std::string ) + 5 * sizeof( std::vector< glm::vec3 > ), "Don't forget to update this function if added/removed members from Model!" );
    model->name = createInfo.name;
    Serializer modelFile;
    if ( !modelFile.OpenForRead( createInfo.filename ) )
    {
        return false;
    }

    std::vector< glm::vec3 > normals;
    std::vector< glm::vec2 > uvs;
    std::vector< glm::vec3 > tangents;
    modelFile.Read( model->vertexPositions );
    modelFile.Read( normals );
    modelFile.Read( uvs );
    modelFile.Read( tangents );
    modelFile.Read( model->indices );
    size_t numMeshes;
    modelFile.Read( numMeshes );
    model->meshes.resize( numMeshes );
    model->originalMaterials.resize( numMeshes );
    for ( size_t i = 0; i < numMeshes; ++i )
    {
        Mesh& mesh = model->meshes[i];
        modelFile.Read( mesh.name );
        std::string matName;
        modelFile.Read( matName ); // should always exist and at least be 'default'
        matNames.push_back( matName );
        modelFile.Read( mesh.startIndex );
        modelFile.Read( mesh.numIndices );
        modelFile.Read( mesh.startVertex );
        modelFile.Read( mesh.numVertices );
    }
    
    model->otherVertexData.resize( model->vertexPositions.size() );
    for ( size_t i = 0; i < model->vertexPositions.size(); ++i )
    {
        model->otherVertexData[i].normal  = normals[i];
        model->otherVertexData[i].uv      = uvs[i];
        model->otherVertexData[i].tangent = tangents[i];
    }

    model->RecalculateNormals();

    return true;
}


bool Model_Load( Model* model, const ModelCreateInfo& createInfo )
{
    std::vector< std::string > materialNames;
    if ( !Model_Load_NoResolveMaterials( model, createInfo, materialNames ) )
    {
        return false;
    }

    // now resolve materials
    model->originalMaterials.resize( materialNames.size() );
    for ( size_t i = 0; i < materialNames.size(); ++i )
    {
        model->originalMaterials[i] = AssetManager::Get< Material >( materialNames[i] );
        if ( !model->originalMaterials[i] )
        {
            LOG_ERR( "No material '%s' found\n", materialNames[i].c_str() );
            return false;
        }
    }

    return true;
}



bool Fastfile_Model_Load( Model* model, Serializer* serializer )
{
    static_assert( sizeof( Model ) == 8 + sizeof( std::string ) + 5 * sizeof( std::vector< glm::vec3 > ), "Don't forget to update this function if added/removed members from Model!" );
    PG_ASSERT( model && serializer );
    serializer->Read( model->name );
    serializer->Read( model->vertexPositions );
    serializer->Read( model->otherVertexData );
    serializer->Read( model->indices );
    size_t numMeshes;
    serializer->Read( numMeshes );
    model->meshes.resize( numMeshes );
    model->originalMaterials.resize( numMeshes );
    for ( size_t i = 0; i < model->meshes.size(); ++i )
    {
        Mesh& mesh = model->meshes[i];
        serializer->Read( mesh.name );
        std::string matName;
        serializer->Read( matName );
        model->originalMaterials[i] = AssetManager::Get< Material >( matName );
        if ( !model->originalMaterials[i] )
        {
            LOG_ERR( "No material found with name '%s', using default material instead.\n", matName.c_str() );
            model->originalMaterials[i] = AssetManager::Get< Material >( "default" );
        }
        serializer->Read( mesh.startIndex );
        serializer->Read( mesh.numIndices );
        serializer->Read( mesh.startVertex );
        serializer->Read( mesh.numVertices );
    }

    return true;
}


bool Fastfile_Model_Save( const Model * const model, Serializer* serializer, const std::vector< std::string >& materialNames )
{
    static_assert( sizeof( Model ) == 8 + sizeof( std::string ) + 5 * sizeof( std::vector< glm::vec3 > ), "Don't forget to update this function if added/removed members from Model!" );
    PG_ASSERT( model && serializer );
    PG_ASSERT( model->meshes.size() == materialNames.size() );

    serializer->Write( model->name );
    serializer->Write( model->vertexPositions );
    serializer->Write( model->otherVertexData );
    serializer->Write( model->indices );
    serializer->Write( model->meshes.size() );
    for ( size_t i = 0; i < model->meshes.size(); ++i )
    {
        const Mesh& mesh = model->meshes[i];
        serializer->Write( mesh.name );
        serializer->Write( materialNames[i] );
        serializer->Write( mesh.startIndex );
        serializer->Write( mesh.numIndices );
        serializer->Write( mesh.startVertex );
        serializer->Write( mesh.numVertices );
    }

    return true;
}


} // namespace Progression