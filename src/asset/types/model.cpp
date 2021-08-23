#include "asset/types/model.hpp"
#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "core/assert.hpp"
#include "renderer/r_globals.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"
#include "glm/geometric.hpp"
#include <cstring>


namespace PG
{

bool Model::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const ModelCreateInfo* createInfo = (const ModelCreateInfo*)baseInfo;
    name = createInfo->name;
    Serializer modelFile;
    if ( !modelFile.OpenForRead( createInfo->filename ) )
    {
        LOG_ERR( "Failed to open PGModel file '%s'", createInfo->filename.c_str() );
        return false;
    }

    modelFile.Read( vertexPositions );
    modelFile.Read( vertexNormals );
    modelFile.Read( vertexTexCoords );
    modelFile.Read( vertexTangents );
    modelFile.Read( indices );
    size_t numMeshes;
    modelFile.Read( numMeshes );
    meshes.resize( numMeshes );
    originalMaterials.resize( numMeshes );
    for ( size_t i = 0; i < numMeshes; ++i )
    {
        Mesh& mesh = meshes[i];
        modelFile.Read( mesh.name );
        std::string matName;
        modelFile.Read( matName ); // should always exist and at least be 'default'
        modelFile.Read( mesh.startIndex );
        modelFile.Read( mesh.numIndices );
        modelFile.Read( mesh.startVertex );
        modelFile.Read( mesh.numVertices );

        originalMaterials[i] = AssetManager::Get< Material >( matName );
        if ( !originalMaterials[i] )
        {
            LOG_ERR( "No material '%s' found", matName.c_str() );
            return false;
        }
    }

    //model->RecalculateNormals();
    UploadToGPU();

    return true;
}


bool Model::FastfileLoad( Serializer* serializer )
{
    PG_STATIC_NDEBUG_ASSERT( sizeof( Model ) == 336, "Don't forget to update this function if added/removed members from Model!" );
    PG_ASSERT( serializer );
    serializer->Read( name );
    serializer->Read( vertexPositions );
    serializer->Read( vertexNormals );
    serializer->Read( vertexTexCoords );
    serializer->Read( vertexTangents );
    serializer->Read( indices );
    size_t numMeshes;
    serializer->Read( numMeshes );
    meshes.resize( numMeshes );
    originalMaterials.resize( numMeshes );
    for ( size_t i = 0; i < meshes.size(); ++i )
    {
        Mesh& mesh = meshes[i];
        serializer->Read( mesh.name );
        std::string matName;
        serializer->Read( matName );
        originalMaterials[i] = AssetManager::Get< Material >( matName );
        if ( !originalMaterials[i] )
        {
            LOG_ERR( "No material found with name '%s', using default material instead.", matName.c_str() );
            originalMaterials[i] = AssetManager::Get< Material >( "default" );
        }
        serializer->Read( mesh.startIndex );
        serializer->Read( mesh.numIndices );
        serializer->Read( mesh.startVertex );
        serializer->Read( mesh.numVertices );
    }

    UploadToGPU();

    return true;
}


bool Model::FastfileSave( Serializer* serializer ) const
{
    PG_STATIC_NDEBUG_ASSERT( sizeof( Model ) == 336, "Don't forget to update this function if added/removed members from Model!" );
    PG_ASSERT( serializer );
    PG_ASSERT( meshes.size() == originalMaterials.size() );

    serializer->Write( name );
    serializer->Write( vertexPositions );
    serializer->Write( vertexNormals );
    serializer->Write( vertexTexCoords );
    serializer->Write( vertexTangents );
    serializer->Write( indices );
    serializer->Write( meshes.size() );
    for ( size_t i = 0; i < meshes.size(); ++i )
    {
        const Mesh& mesh = meshes[i];
        serializer->Write( mesh.name );
        serializer->Write( originalMaterials[i]->name );
        serializer->Write( mesh.startIndex );
        serializer->Write( mesh.numIndices );
        serializer->Write( mesh.startVertex );
        serializer->Write( mesh.numVertices );
    }

    return true;
}


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
        glm::vec3 e01 = vertexPositions[i1] - vertexPositions[i0];
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


} // namespace PG