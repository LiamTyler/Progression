#include "asset/types/model.hpp"
#include "asset/types/material.hpp"
#include "asset/asset_manager.hpp"
#include "renderer/r_globals.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include "glm/geometric.hpp"
#include <cstring>


namespace PG
{

std::string GetAbsPath_ModelFilename( const std::string& filename )
{
    return PG_ASSET_DIR + filename;
}

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
    std::ifstream in( GetAbsPath_ModelFilename( createInfo->filename ) );
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
    if ( materialNames.size() != numMeshes )
    {
        materialNames.resize( numMeshes, "default" );
    }

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
        if ( createInfo->flipTexCoordsVertically )
        {
            texCoords[i].y = 1.0f - texCoords[i].y;
        }
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

    if ( createInfo->recalculateNormals )
    {
        RecalculateNormals();
    }
    UploadToGPU();

    return true;
}


bool Model::FastfileLoad( Serializer* serializer )
{
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
        originalMaterials[i] = AssetManager::Get<Material>( matName );
        if ( !originalMaterials[i] )
        {
            LOG_ERR( "No material found with name '%s', using default material instead.", matName.c_str() );
            originalMaterials[i] = AssetManager::Get<Material>( "default" );
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
    newNormals.resize( normals.size(), glm::vec3( 0 ) );
    for ( const Mesh& mesh : meshes )
    {
        for ( uint32_t i = 0; i < mesh.numIndices; i += 3 )
        {
            const auto i0 = mesh.startVertex + indices[mesh.startIndex + i + 0];
            const auto i1 = mesh.startVertex + indices[mesh.startIndex + i + 1];
            const auto i2 = mesh.startVertex + indices[mesh.startIndex + i + 2];
            glm::vec3 e01 = positions[i1] - positions[i0];
            glm::vec3 e02 = positions[i2] - positions[i0];
            auto N = glm::cross( e01, e02 );
            if ( glm::length( N ) <= 0.00001f )
            {
                // degenerate tri, just use the existing normals
                newNormals[i0] += normals[i0];
                newNormals[i1] += normals[i1];
                newNormals[i2] += normals[i2];
            }
            else
            {
                glm::vec3 n = glm::normalize( N );
                newNormals[i0] += n;
                newNormals[i1] += n;
                newNormals[i2] += n;
            }
        }
    }

    for ( size_t i = 0; i < normals.size(); ++i )
    {
        normals[i] = glm::normalize( newNormals[i] );
    }
}


void Model::CreateBLAS()
{
#if USING( PG_RTX ) && USING( GPU_DATA )
    VkDeviceAddress vertexAddress = vertexBuffer.GetDeviceAddress();
    VkDeviceAddress indexAddress  = indexBuffer.GetDeviceAddress();
    uint32_t numTriangles = static_cast<uint32_t>( (indexBuffer.GetLength() / sizeof( uint32_t )) / 3 );

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = numVertices;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof( glm::vec3 );
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexAddress;
    accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizesKHR( Gfx::rg.device.GetHandle(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo, &numTriangles, &accelerationStructureBuildSizesInfo );

    using namespace Gfx;
    blas = rg.device.NewAccelerationStructure( AccelerationStructureType::BLAS, accelerationStructureBuildSizesInfo.accelerationStructureSize );

    Buffer scratchBuffer = rg.device.NewBuffer( accelerationStructureBuildSizesInfo.buildScratchSize, BUFFER_TYPE_STORAGE | BUFFER_TYPE_DEVICE_ADDRESS, MEMORY_TYPE_DEVICE_LOCAL );

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = blas.GetHandle();
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    CommandBuffer cmdBuf = rg.commandPools[GFX_CMD_POOL_TRANSIENT].NewCommandBuffer( "One time Build AS" );
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
    vkCmdBuildAccelerationStructuresKHR( cmdBuf.GetHandle(), 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data() );
    cmdBuf.EndRecording();
    rg.device.Submit( cmdBuf );
    rg.device.WaitForIdle();
    cmdBuf.Free();
    scratchBuffer.Free();
#endif // #if USING( PG_RTX ) && USING( GPU_DATA )
}


void Model::UploadToGPU()
{
#if USING( GPU_DATA )
    FreeGPU();
    numVertices = static_cast<uint32_t>( positions.size() );

    PG_ASSERT( positions.size() == normals.size() );

    // TODO: allow for variable-attribute meshes. For now, just allocate a zero buffer for texCoords and tangents if they are not specified
    PG_ASSERT( tangents.empty() || positions.size() == tangents.size() );
    PG_ASSERT( texCoords.empty() || positions.size() == texCoords.size() );
    size_t texCoordCount = positions.size();
    size_t tangentCount = positions.size();

    size_t totalSize = 0;
    totalSize += positions.size() * sizeof( glm::vec3 );
    totalSize += normals.size() * sizeof( glm::vec3 );
    totalSize += texCoordCount * sizeof( glm::vec2 );
    totalSize += tangentCount * sizeof( glm::vec3 );
    if ( totalSize == 0 && indices.size() == 0 )
    {
        return;
    }

    gpuPositionOffset = 0;
    size_t offset = 0;
    unsigned char* tmpMem = static_cast< unsigned char* >( malloc( totalSize ) );
    memcpy( tmpMem + offset, positions.data(), positions.size() * sizeof( glm::vec3 ) );
    offset += positions.size() * sizeof( glm::vec3 );

    gpuNormalOffset = offset;
    memcpy( tmpMem + offset, normals.data(), normals.size() * sizeof( glm::vec3 ) );
    offset += normals.size() * sizeof( glm::vec3 );

    gpuTexCoordOffset = offset;
    if ( texCoords.empty() )
    {
        memset( tmpMem + offset, 0, texCoordCount * sizeof( glm::vec2 ) );
    }
    else
    {
        memcpy( tmpMem + offset, texCoords.data(), texCoordCount * sizeof( glm::vec2 ) );
    }
    offset += texCoordCount * sizeof( glm::vec2 );
    
    gpuTangentOffset = offset;
    if ( tangents.empty() )
    {
        memset( tmpMem + offset, 0, tangentCount * sizeof( glm::vec2 ) );
    }
    else
    {
        memcpy( tmpMem + offset, tangents.data(), tangentCount * sizeof( glm::vec2 ) );
    }
    offset += tangentCount * sizeof( glm::vec3 );

    using namespace Gfx;
    BufferType rayTracingType = BUFFER_TYPE_DEVICE_ADDRESS | BUFFER_TYPE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY | BUFFER_TYPE_STORAGE;
    vertexBuffer = rg.device.NewBuffer( totalSize, tmpMem, rayTracingType | BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "Vertex, model: " + name );
    indexBuffer = rg.device.NewBuffer( indices.size() * sizeof( uint32_t ), indices.data(), rayTracingType | BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, "Index, model: " + name );
    free( tmpMem );
    CreateBLAS();

    FreeCPU();
#endif // #if USING( GPU_DATA )
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
#if USING( GPU_DATA )
    if ( blas )
    {
        blas.Free();
    }
    if ( vertexBuffer )
    {
        vertexBuffer.Free();
    }
    if ( indexBuffer)
    {
        indexBuffer.Free();
    }
#endif // #if USING( GPU_DATA )
}


std::vector<std::string> GetModelMaterialList( const std::string& fullModelPath )
{
    std::vector<std::string> materials;
    if ( GetFileExtension( fullModelPath ) != ".pmodel" )
    {
        LOG_ERR( "Model::Load only takes .pmodel format" );
        return materials;
    }
    std::ifstream in( fullModelPath );
    if ( !in )
    {
        LOG_ERR( "Failed to open .pmodel file '%s'", fullModelPath.c_str() );
        return materials;
    }

    std::string tmp;
    uint32_t numMaterials;
    in >> tmp >> numMaterials;
    materials.resize( numMaterials );
    for ( uint32_t i = 0; i < numMaterials; ++i )
    {
        in >> tmp >> materials[i];
    }

    return materials;
}


} // namespace PG