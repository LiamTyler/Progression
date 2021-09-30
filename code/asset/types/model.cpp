#include "asset/types/model.hpp"
#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "shared/assert.hpp"
#include "renderer/r_globals.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "glm/geometric.hpp"
#include <cstring>
#include <sstream>


namespace PG
{

bool Model::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const ModelCreateInfo* createInfo = (const ModelCreateInfo*)baseInfo;
    name = createInfo->name;
    if ( GetFileExtension( createInfo->filename ) != ".pmodel" )
    {
        LOG_ERR( "Model::Load only takes .pmodel format" );
        return false;
    }
    std::ifstream in( createInfo->filename );
    if ( !in )
    {
        LOG_ERR( "Failed to open .pmodel file '%s'", createInfo->filename.c_str() );
        return false;
    }

    std::string tmp;
    uint32_t numMaterials;
    in >> tmp >> numMaterials;
    std::vector<std::string> materialNames( numMaterials );
    for ( uint32_t i = 0; i < numMaterials; ++i )
    {
        in >> tmp >> materialNames[i];
    }

    uint32_t numMeshes;
    in >> tmp >> numMeshes;
    meshes.resize( numMeshes );
    originalMaterials.resize( numMeshes );
    for ( uint32_t i = 0; i < numMeshes; ++i )
    {
        int materialIdx;
        in >> tmp >> meshes[i].name;
        in >> tmp >> materialIdx;
        in >> tmp >> meshes[i].startIndex;
        in >> tmp >> meshes[i].startVertex;
        in >> tmp >> meshes[i].numIndices;
        in >> tmp >> meshes[i].numVertices;
        meshes[i].startIndex *= 3;
        meshes[i].numIndices *= 3;

        originalMaterials[i] = AssetManager::Get<Material>( materialNames[materialIdx] );
        if ( !originalMaterials[i] )
        {
            LOG_ERR( "No material '%s' found", materialNames[materialIdx].c_str() );
            return false;
        }
    }
    in >> tmp;

    uint32_t numPositions;
    in >> tmp >> numPositions; PG_ASSERT( numPositions < 100000000 && in.good() );
    positions.resize( numPositions );
    for ( uint32_t i = 0; i < numPositions; ++i )
    {
        in >> positions[i].x >> positions[i].y >> positions[i].z;
    }

    uint32_t numNormals;
    in >> tmp >> numNormals; PG_ASSERT( numNormals < 100000000 && in.good() );
    normals.resize( numNormals );
    for ( uint32_t i = 0; i < numNormals; ++i )
    {
        in >> normals[i].x >> normals[i].y >> normals[i].z;
    }

    uint32_t numTexCoords;
    in >> tmp >> numTexCoords;
    texCoords.resize( numTexCoords ); PG_ASSERT( numTexCoords < 100000000 && in.good() );
    for ( uint32_t i = 0; i < numTexCoords; ++i )
    {
        in >> texCoords[i].x >> texCoords[i].y;
    }

    uint32_t numTangents;
    in >> tmp >> numTangents; PG_ASSERT( numTangents < 100000000 && in.good() );
    tangents.resize( numTangents );
    for ( uint32_t i = 0; i < numTangents; ++i )
    {
        in >> tangents[i].x >> tangents[i].y >> tangents[i].z;
    }

    uint32_t numTris;
    in >> tmp >> numTris; PG_ASSERT( numTris < 100000000 && in.good() );
    indices.resize( numTris * 3 );
    for ( uint32_t i = 0; i < numTris; ++i )
    {
        in >> indices[3*i + 0] >> indices[3*i + 1] >> indices[3*i + 2];
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
    serializer->Read( positions );
    serializer->Read( normals );
    serializer->Read( texCoords );
    serializer->Read( tangents );
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
    serializer->Write( positions );
    serializer->Write( normals );
    serializer->Write( texCoords );
    serializer->Write( tangents );
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
    PG_ASSERT( positions.size() == normals.size() );
    std::vector< glm::vec3 > newNormals;
    newNormals.resize( positions.size(), glm::vec3( 0 ) );
    for ( size_t i = 0; i < indices.size(); i += 3 )
    {
        const auto i0 = indices[i + 0];
        const auto i1 = indices[i + 1];
        const auto i2 = indices[i + 2];
        glm::vec3 e01 = positions[i1] - positions[i0];
        glm::vec3 e02 = positions[i2] - positions[i0];
        glm::vec3 n = glm::normalize( glm::cross( e01, e02 ) );
        newNormals[i0] += n;
        newNormals[i1] += n;
        newNormals[i2] += n;
    }

    for ( size_t i = 0; i < positions.size(); ++i )
    {
        normals[i] = glm::normalize( newNormals[i] );
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
    totalSize += positions.size() * sizeof( glm::vec3 );
    totalSize += normals.size() * sizeof( glm::vec3 );
    totalSize += texCoords.size() * sizeof( glm::vec2 );
    totalSize += tangents.size() * sizeof( glm::vec3 );
    if ( totalSize == 0 && indices.size() == 0 )
    {
        return;
    }

    gpuPositionOffset = 0;
    size_t offset = 0;
    unsigned char* tmpMem = static_cast< unsigned char* >( malloc( totalSize ) );
    memcpy( tmpMem + offset, positions.data(), positions.size() * sizeof( glm::vec3 ) );
    offset += static_cast< uint32_t >( positions.size() ) * sizeof( glm::vec3 );

    gpuNormalOffset = offset;
    memcpy( tmpMem + offset, normals.data(), normals.size() * sizeof( glm::vec3 ) );
    offset += static_cast< uint32_t >( normals.size() ) * sizeof( glm::vec3 );

    gpuTexCoordOffset = offset;
    memcpy( tmpMem + offset, texCoords.data(), texCoords.size() * sizeof( glm::vec2 ) );
    offset += static_cast< uint32_t >( texCoords.size() ) * sizeof( glm::vec2 );
    
    gpuTangentOffset = offset;
    memcpy( tmpMem + offset, tangents.data(), tangents.size() * sizeof( glm::vec3 ) );
    offset += static_cast< uint32_t >( tangents.size() ) * sizeof( glm::vec3 );

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
    positions.clear(); positions.shrink_to_fit();
    normals.clear(); normals.shrink_to_fit();
    texCoords.clear(); texCoords.shrink_to_fit();
    tangents.clear(); tangents.shrink_to_fit();
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