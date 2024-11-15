#include "asset/types/model.hpp"
#include "asset/asset_manager.hpp"
#include "asset/pmodel.hpp"
#include "asset/types/material.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <cstring>

#if USING( CONVERTER )
#include "meshoptimizer/src/meshoptimizer.h"
#endif // #if USING( CONVERTER )
#if USING( GPU_DATA )
using namespace PG::Gfx;
#include "renderer/graphics_api/buffer.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "renderer/r_globals.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

std::string GetAbsPath_ModelFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

bool Model::Load( const BaseAssetCreateInfo* baseInfo )
{
#if USING( CONVERTER )
    PG_ASSERT( baseInfo );
    const ModelCreateInfo* createInfo = (const ModelCreateInfo*)baseInfo;
    SetName( createInfo->name );

    PModel pmodel;
    if ( !pmodel.Load( GetAbsPath_ModelFilename( createInfo->filename ) ) )
        return false;

    meshes.resize( pmodel.meshes.size() );
    meshAABBs.resize( pmodel.meshes.size() );
    for ( u32 meshIdx = 0; meshIdx < (u32)meshes.size(); ++meshIdx )
    {
        Mesh& m                   = meshes[meshIdx];
        const PModel::Mesh& pMesh = pmodel.meshes[meshIdx];

        m.name     = pMesh.name;
        m.material = AssetManager::Get<Material>( pMesh.materialName );
        if ( !m.material )
        {
            LOG_ERR( "No material '%s' found", pMesh.materialName.c_str() );
            return false;
        }

        constexpr float CONE_WEIGHT = 0.5f;

        size_t maxMeshlets = meshopt_buildMeshletsBound( pMesh.indices.size(), MAX_VERTS_PER_MESHLET, MAX_TRIS_PER_MESHLET );
        std::vector<meshopt_Meshlet> meshlets( maxMeshlets );
        std::vector<u32> meshletVertices( maxMeshlets * MAX_VERTS_PER_MESHLET );
        std::vector<u8> meshletTriangles( maxMeshlets * MAX_TRIS_PER_MESHLET * 3 );

        size_t meshletCount = meshopt_buildMeshlets( meshlets.data(), meshletVertices.data(), meshletTriangles.data(), pMesh.indices.data(),
            pMesh.indices.size(), &pMesh.vertices[0].pos.x, pMesh.vertices.size(), sizeof( PModel::Vertex ), MAX_VERTS_PER_MESHLET,
            MAX_TRIS_PER_MESHLET, CONE_WEIGHT );

        if ( meshletCount > 65535 )
        {
            LOG_ERR( "About 50%% of gpus have a maxMeshWorkGroupCount.x of 65535. To support this mesh, either add code for splitting the "
                     "mesh based on this count, or adding runtime support for dispatching with the Y/Z dimension as well" );
            return false;
        }

        m.meshlets.resize( meshletCount );
        m.meshletCullDatas.resize( meshletCount );
        u32 numVerts = 0;
        u32 numTris  = 0;
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const meshopt_Meshlet& meshlet = meshlets[meshletIdx];
            GpuData::Meshlet& pgMeshlet    = m.meshlets[meshletIdx];
            meshopt_optimizeMeshlet( &meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset],
                meshlet.triangle_count, meshlet.vertex_count );

            PG_ASSERT( numVerts == meshlet.vertex_offset );
            pgMeshlet.vertexOffset  = meshlet.vertex_offset;
            pgMeshlet.triOffset     = numTris;
            pgMeshlet.vertexCount   = meshlet.vertex_count;
            pgMeshlet.triangleCount = meshlet.triangle_count;

            numVerts += pgMeshlet.vertexCount;
            numTris += pgMeshlet.triangleCount;

            meshopt_Bounds bounds =
                meshopt_computeMeshletBounds( &meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset],
                    meshlet.triangle_count, &pMesh.vertices[0].pos.x, meshlet.vertex_count, sizeof( PModel::Vertex ) );

            GpuData::PackedMeshletCullData& cullData = m.meshletCullDatas[meshletIdx];
            // cullData.positionXY             = Float32ToFloat16( bounds.center[0], bounds.center[1] );
            // cullData.positionZAndRadius     = Float32ToFloat16( bounds.center[2], bounds.radius );
            // cullData.coneAxisAndCutoff      = 0;
            // cullData.coneAxisAndCutoff |= ( bounds.cone_axis_s8[0] << 0 );
            // cullData.coneAxisAndCutoff |= ( bounds.cone_axis_s8[1] << 8 );
            // cullData.coneAxisAndCutoff |= ( bounds.cone_axis_s8[2] << 16 );
            // cullData.coneAxisAndCutoff |= ( bounds.cone_cutoff_s8 << 24 );
            // cullData._padTo16Bytes = 0;

            cullData.position   = vec3( bounds.center[0], bounds.center[1], bounds.center[2] );
            cullData.radius     = bounds.radius;
            cullData.coneAxis   = vec3( bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2] );
            cullData.coneCutoff = bounds.cone_cutoff;
            cullData.coneApex   = vec3( bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2] );
        }

        m.positions.resize( numVerts );
        m.normals.resize( numVerts );
        if ( pMesh.numUVChannels > 0 )
            m.texCoords.resize( numVerts );
        if ( pMesh.hasTangents )
            m.tangents.resize( numVerts );
        m.indices.resize( 4 * numTris ); // for now, storing 1 byte of padding after every 3 indices, for easier indexing

        u32 numZeroNormals  = 0;
        u32 numZeroTangents = 0;
        AABB& meshAABB      = meshAABBs[meshIdx];
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const meshopt_Meshlet& moMeshlet  = meshlets[meshletIdx];
            const GpuData::Meshlet& pgMeshlet = m.meshlets[meshletIdx];
            for ( u8 localVIdx = 0; localVIdx < pgMeshlet.vertexCount; ++localVIdx )
            {
                size_t globalMOIdx       = moMeshlet.vertex_offset + localVIdx;
                size_t globalPGIdx       = pgMeshlet.vertexOffset + localVIdx;
                const PModel::Vertex& v  = pMesh.vertices[meshletVertices[globalMOIdx]];
                m.positions[globalPGIdx] = v.pos;
                meshAABB.Encompass( v.pos );
                m.normals[globalPGIdx] = v.normal;
                if ( Length( v.normal ) <= 0.00001f )
                    ++numZeroNormals;

                if ( pMesh.numUVChannels > 0 )
                {
                    m.texCoords[globalPGIdx] = v.uvs[0];
                    if ( createInfo->flipTexCoordsVertically )
                        m.texCoords[globalPGIdx].y = 1.0f - v.uvs[0].y;
                }
                if ( pMesh.hasTangents )
                {
                    vec3 tangent            = v.tangent;
                    vec3 bitangent          = v.bitangent;
                    vec3 tNormal            = Cross( tangent, bitangent );
                    float bSign             = Dot( v.normal, tNormal ) > 0.0f ? 1.0f : -1.0f;
                    vec4 packed             = vec4( tangent, bSign );
                    m.tangents[globalPGIdx] = packed;
                    if ( Length( v.tangent ) <= 0.00001f )
                        ++numZeroTangents;
                }
            }

            for ( u8 localTriIdx = 0; localTriIdx < pgMeshlet.triangleCount; ++localTriIdx )
            {
                size_t globalMOIdx         = moMeshlet.triangle_offset + 3 * localTriIdx;
                size_t globalPGIdx         = 4 * pgMeshlet.triOffset + 4 * localTriIdx;
                m.indices[globalPGIdx + 0] = meshletTriangles[globalMOIdx + 0];
                m.indices[globalPGIdx + 1] = meshletTriangles[globalMOIdx + 1];
                m.indices[globalPGIdx + 2] = meshletTriangles[globalMOIdx + 2];
                m.indices[globalPGIdx + 3] = 0;
            }
        }
        if ( numZeroNormals )
            LOG_WARN( "Mesh[%u] '%s' inside of model '%s' has %u normals of length 0", meshIdx, pMesh.name.c_str(),
                createInfo->name.c_str(), numZeroNormals );
        if ( numZeroTangents )
            LOG_WARN( "Mesh[%u] '%s' inside of model '%s' has %u tangents of length 0", meshIdx, pMesh.name.c_str(),
                createInfo->name.c_str(), numZeroTangents );
    }
    PG_ASSERT( !createInfo->recalculateNormals, "Not implemented with meshlets yet" );

    return true;
#else  // #if USING( CONVERTER )
    return false;
#endif // #else // #if USING( CONVERTER )
}

bool Model::FastfileLoad( Serializer* serializer )
{
    PGP_ZONE_SCOPED_ASSET( "Model::FastfileLoad" );
    u32 numMeshes = serializer->Read<u32>();
    meshAABBs.resize( numMeshes );
    serializer->Read( meshAABBs.data(), numMeshes * sizeof( AABB ) );
    meshes.resize( numMeshes );
    for ( Mesh& mesh : meshes )
    {
#if USING( ASSET_NAMES )
        serializer->Read<u16>( mesh.name );
#else  // #if USING( ASSET_NAMES )
        u16 meshNameLen;
        serializer->Read( meshNameLen );
        serializer->Skip( meshNameLen );
#endif // #else // #if USING( ASSET_NAMES )
        std::string matName;
        serializer->Read<u16>( matName );
        mesh.material = AssetManager::Get<Material>( matName );
        if ( !mesh.material )
        {
            LOG_ERR( "No material found with name '%s', using default material instead.", matName.c_str() );
            mesh.material = AssetManager::Get<Material>( "default" );
        }

#if !USING( GPU_DATA )
        serializer->Read( mesh.positions );
        serializer->Read( mesh.normals );
        serializer->Read( mesh.texCoords );
        serializer->Read( mesh.tangents );
        serializer->Read( mesh.indices );
        serializer->Read( mesh.meshlets );
        serializer->Read( mesh.meshletCullDatas );
#else // #if !USING( GPU_DATA )
        size_t numPos, numNormal, numTexCoords, numTan, numIndices, numMeshlets;
        const void *posData, *normalData, *uvData, *tanData, *indexData, *meshletData, *meshletCullData;
        serializer->Read( numPos );
        posData = serializer->GetData();
        serializer->Skip( numPos * sizeof( vec3 ) );

        serializer->Read( numNormal );
        normalData = serializer->GetData();
        serializer->Skip( numNormal * sizeof( vec3 ) );

        serializer->Read( numTexCoords );
        uvData = serializer->GetData();
        serializer->Skip( numTexCoords * sizeof( vec2 ) );

        serializer->Read( numTan );
        tanData = serializer->GetData();
        serializer->Skip( numTan * sizeof( vec4 ) );

        serializer->Read( numIndices );
        indexData = serializer->GetData();
        serializer->Skip( numIndices * sizeof( u8 ) );
        PG_ASSERT( numIndices % 4 == 0 );

        serializer->Read( numMeshlets );
        meshletData = serializer->GetData();
        serializer->Skip( numMeshlets * sizeof( GpuData::Meshlet ) );

        meshletCullData = serializer->GetData();
        serializer->Skip( numMeshlets * sizeof( GpuData::PackedMeshletCullData ) );

        mesh.numVertices  = static_cast<u32>( numPos );
        mesh.numMeshlets  = static_cast<u32>( numMeshlets );
        mesh.hasTexCoords = numTexCoords != 0;
        mesh.hasTangents  = numTan != 0;
        PG_ASSERT( numMeshlets <= 65535, "Vulkan limits. Split mesh during converter if this isn't true" );

        size_t totalVertexSizeInBytes = 0;
        totalVertexSizeInBytes += numPos * sizeof( vec3 );
        totalVertexSizeInBytes += numNormal * sizeof( vec3 );
        totalVertexSizeInBytes += numTexCoords * sizeof( vec2 );
        totalVertexSizeInBytes += numTan * sizeof( vec4 );

        BufferCreateInfo vbCreateInfo{};
        vbCreateInfo.size               = totalVertexSizeInBytes;
        vbCreateInfo.bufferUsage        = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        vbCreateInfo.addToBindlessArray = false; // since BindlessManager::AddMeshBuffers will take care of it

        BufferCreateInfo tbCreateInfo = vbCreateInfo;
        tbCreateInfo.size             = numIndices * sizeof( u8 );

        BufferCreateInfo mbCreateInfo = vbCreateInfo;
        mbCreateInfo.size             = numMeshlets * sizeof( GpuData::Meshlet );

        BufferCreateInfo cbCreateInfo = vbCreateInfo;
        cbCreateInfo.size             = numMeshlets * sizeof( GpuData::PackedMeshletCullData );

        mesh.buffers = new Gfx::Buffer[Mesh::BUFFER_COUNT];
#if USING( ASSET_NAMES )
        mesh.buffers[Mesh::VERTEX_BUFFER]    = rg.device.NewBuffer( vbCreateInfo, std::string( m_name ) + mesh.name + "_vb" );
        mesh.buffers[Mesh::TRI_BUFFER]       = rg.device.NewBuffer( tbCreateInfo, std::string( m_name ) + mesh.name + "_tb" );
        mesh.buffers[Mesh::MESHLET_BUFFER]   = rg.device.NewBuffer( mbCreateInfo, std::string( m_name ) + mesh.name + "_mb" );
        mesh.buffers[Mesh::CULL_DATA_BUFFER] = rg.device.NewBuffer( cbCreateInfo, std::string( m_name ) + mesh.name + "_mcb" );
#else  // #if USING( ASSET_NAMES )
        mesh.buffers[Mesh::VERTEX_BUFFER]    = rg.device.NewBuffer( vbCreateInfo );
        mesh.buffers[Mesh::TRI_BUFFER]       = rg.device.NewBuffer( tbCreateInfo );
        mesh.buffers[Mesh::MESHLET_BUFFER]   = rg.device.NewBuffer( mbCreateInfo );
        mesh.buffers[Mesh::CULL_DATA_BUFFER] = rg.device.NewBuffer( cbCreateInfo );
#endif // #else // #if USING( ASSET_NAMES )
        mesh.bindlessBuffersSlot = BindlessManager::AddMeshBuffers( &mesh );

        size_t offset = 0;
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], posData, numPos * sizeof( vec3 ), offset );
        offset += numPos * sizeof( vec3 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], normalData, numNormal * sizeof( vec3 ), offset );
        offset += numNormal * sizeof( vec3 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], uvData, numTexCoords * sizeof( vec2 ), offset );
        offset += numTexCoords * sizeof( vec2 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], tanData, numTan * sizeof( vec4 ), offset );

        rg.device.AddUploadRequest( mesh.buffers[Mesh::TRI_BUFFER], indexData, tbCreateInfo.size );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::MESHLET_BUFFER], meshletData, mbCreateInfo.size );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::CULL_DATA_BUFFER], meshletCullData, cbCreateInfo.size );
#endif // #else // #if !USING( GPU_DATA )
    }

    return true;
}

bool Model::FastfileSave( Serializer* serializer ) const
{
#if !USING( GPU_DATA )
    SerializeName( serializer );
    serializer->Write( (u32)meshes.size() );
    serializer->Write( meshAABBs.data(), meshAABBs.size() * sizeof( AABB ) );
    for ( const Mesh& mesh : meshes )
    {
        serializer->Write<u16>( mesh.name );
        serializer->Write<u16>( mesh.material->GetName() );
        serializer->Write( mesh.positions );
        serializer->Write( mesh.normals );
        serializer->Write( mesh.texCoords );
        serializer->Write( mesh.tangents );
        serializer->Write( mesh.indices );
        serializer->Write( mesh.meshlets );
        serializer->Write( mesh.meshletCullDatas.data(), mesh.meshletCullDatas.size() * sizeof( GpuData::PackedMeshletCullData ) );
    }
#endif // #if !USING( GPU_DATA )

    return true;
}

void Model::Free()
{
    FreeGPU();
    FreeCPU();
}

void Model::FreeCPU()
{
#if !USING( GPU_DATA )
    for ( Mesh& mesh : meshes )
    {
        mesh.meshletCullDatas = std::vector<GpuData::PackedMeshletCullData>();
        mesh.meshlets         = std::vector<GpuData::Meshlet>();
        mesh.positions        = std::vector<vec3>();
        mesh.normals          = std::vector<vec3>();
        mesh.texCoords        = std::vector<vec2>();
        mesh.tangents         = std::vector<vec4>();
        mesh.indices          = std::vector<u8>();
    }
#endif // #if !USING( GPU_DATA )
}

void Model::FreeGPU()
{
#if USING( GPU_DATA )
    for ( Mesh& mesh : meshes )
    {
        if ( mesh.bindlessBuffersSlot != PG_INVALID_BUFFER_INDEX )
        {
            Gfx::BindlessManager::RemoveMeshBuffers( &mesh );
            DEBUG_BUILD_ONLY( mesh.bindlessBuffersSlot = PG_INVALID_BUFFER_INDEX );
        }
        if ( mesh.buffers )
        {
            for ( int i = 0; i < Mesh::BUFFER_COUNT; ++i )
                mesh.buffers[i].Free();
            delete[] mesh.buffers;
        }
    }
#endif // #if USING( GPU_DATA )
}

} // namespace PG
