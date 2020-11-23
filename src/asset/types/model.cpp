#include "asset/types/model.hpp"
#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "core/assert.hpp"
#include "renderer/r_globals.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "glm/geometric.hpp"


namespace PG
{


void Model::RecalculateNormals()
{
    PG_ASSERT( vertexPositions.size() == vertexNormals.size() );
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
        vertexNormals[i] = glm::normalize( newNormals[i] );
    }
}


void Model::UploadToGPU()
{
#if !USING( COMPILING_CONVERTER )
    if ( vertexBuffer )
    {
        vertexBuffer.Free();
    }
    if ( indexBuffer)
    {
        indexBuffer.Free();
    }

    size_t totalSize = 0;
    totalSize += vertexPositions.size() * sizeof( glm::vec3 );
    totalSize += vertexNormals.size() * sizeof( glm::vec3 );
    totalSize += vertexTexCoords.size() * sizeof( glm::vec2 );
    totalSize += vertexTangents.size() * sizeof( glm::vec3 );
    if ( totalSize == 0 && indices.size() == 0 )
    {
        return;
    }

    gpuPositionOffset = 0;
    size_t offset = 0;
    unsigned char* tmpMem = static_cast< unsigned char* >( malloc( totalSize ) );
    memcpy( tmpMem + offset, vertexPositions.data(), vertexPositions.size() * sizeof( glm::vec3 ) );
    offset += static_cast< uint32_t >( vertexPositions.size() ) * sizeof( glm::vec3 );

    gpuNormalOffset = offset;
    memcpy( tmpMem + offset, vertexNormals.data(), vertexNormals.size() * sizeof( glm::vec3 ) );
    offset += static_cast< uint32_t >( vertexNormals.size() ) * sizeof( glm::vec3 );

    gpuTexCoordOffset = offset;
    memcpy( tmpMem + offset, vertexTexCoords.data(), vertexTexCoords.size() * sizeof( glm::vec2 ) );
    offset += static_cast< uint32_t >( vertexTexCoords.size() ) * sizeof( glm::vec2 );
    
    gpuTangentOffset = offset;
    memcpy( tmpMem + offset, vertexTangents.data(), vertexTangents.size() * sizeof( glm::vec3 ) );
    offset += static_cast< uint32_t >( vertexTangents.size() ) * sizeof( glm::vec3 );

    vertexBuffer = Gfx::r_globals.device.NewBuffer( totalSize, tmpMem, Gfx::BUFFER_TYPE_VERTEX, Gfx::MEMORY_TYPE_DEVICE_LOCAL, "Vertex, model: " + name );
    indexBuffer = Gfx::r_globals.device.NewBuffer( indices.size() * sizeof( uint32_t ), indices.data(), Gfx::BUFFER_TYPE_INDEX, Gfx::MEMORY_TYPE_DEVICE_LOCAL, "Index, model: " + name );
    free( tmpMem );

    FreeCPU();
#endif // #if !USING( COMPILING_CONVERTER )
}


void Model::Free()
{
    FreeGPU();
    FreeCPU();
}


void Model::FreeCPU()
{
    vertexPositions.clear(); vertexPositions.shrink_to_fit();
    vertexNormals.clear(); vertexNormals.shrink_to_fit();
    vertexTexCoords.clear(); vertexTexCoords.shrink_to_fit();
    vertexTangents.clear(); vertexTangents.shrink_to_fit();
}


void Model::FreeGPU()
{
#if !USING( COMPILING_CONVERTER )
    if ( vertexBuffer )
    {
        vertexBuffer.Free();
    }
    if ( indexBuffer)
    {
        indexBuffer.Free();
    }
#endif // #if !USING( COMPILING_CONVERTER )
}


bool Model_Load_PGModel( Model* model, const ModelCreateInfo& createInfo, std::vector< std::string >& matNames )
{
    PG_STATIC_NDEBUG_ASSERT( sizeof( Model ) == 336, "Don't forget to update this function if added/removed members from Model!" );
    model->name = createInfo.name;
    Serializer modelFile;
    if ( !modelFile.OpenForRead( createInfo.filename ) )
    {
        return false;
    }

    modelFile.Read( model->vertexPositions );
    modelFile.Read( model->vertexNormals );
    modelFile.Read( model->vertexTexCoords );
    modelFile.Read( model->vertexTangents );
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

    model->RecalculateNormals();

    return true;
}


bool Model_Load( Model* model, const ModelCreateInfo& createInfo )
{
    std::vector< std::string > materialNames;
    if ( !Model_Load_PGModel( model, createInfo, materialNames ) )
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

    model->UploadToGPU();

    return true;
}


bool Fastfile_Model_Load( Model* model, Serializer* serializer )
{
    PG_STATIC_NDEBUG_ASSERT( sizeof( Model ) == 336, "Don't forget to update this function if added/removed members from Model!" );
    PG_ASSERT( model && serializer );
    serializer->Read( model->name );
    serializer->Read( model->vertexPositions );
    serializer->Read( model->vertexNormals );
    serializer->Read( model->vertexTexCoords );
    serializer->Read( model->vertexTangents );
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

    model->UploadToGPU();

    return true;
}


bool Fastfile_Model_Save( const Model * const model, Serializer* serializer, const std::vector< std::string >& materialNames )
{
    PG_STATIC_NDEBUG_ASSERT( sizeof( Model ) == 336, "Don't forget to update this function if added/removed members from Model!" );
    PG_ASSERT( model && serializer );
    PG_ASSERT( model->meshes.size() == materialNames.size() );

    serializer->Write( model->name );
    serializer->Write( model->vertexPositions );
    serializer->Write( model->vertexNormals );
    serializer->Write( model->vertexTexCoords );
    serializer->Write( model->vertexTangents );
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


} // namespace PG