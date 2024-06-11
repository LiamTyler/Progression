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

        constexpr size_t MAX_VERTS_PER_MESHLET = 64;
        constexpr size_t MAX_TRIS_PER_MESHLET  = 124;
        constexpr float CONE_WEIGHT            = 0.5f;

        size_t maxMeshlets = meshopt_buildMeshletsBound( pMesh.indices.size(), MAX_VERTS_PER_MESHLET, MAX_TRIS_PER_MESHLET );
        std::vector<meshopt_Meshlet> meshlets( maxMeshlets );
        std::vector<u32> meshletVertices( maxMeshlets * MAX_VERTS_PER_MESHLET );
        std::vector<u8> meshletTriangles( maxMeshlets * MAX_TRIS_PER_MESHLET * 3 );

        size_t meshletCount = meshopt_buildMeshlets( meshlets.data(), meshletVertices.data(), meshletTriangles.data(), pMesh.indices.data(),
            pMesh.indices.size(), &pMesh.vertices[0].pos.x, pMesh.vertices.size(), sizeof( PModel::Vertex ), MAX_VERTS_PER_MESHLET,
            MAX_TRIS_PER_MESHLET, CONE_WEIGHT );

        m.meshlets.resize( meshletCount );
        u32 numVerts = 0;
        u32 numTris  = 0;
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const meshopt_Meshlet& meshlet = meshlets[meshletIdx];
            Meshlet& pgMeshlet             = m.meshlets[meshletIdx];
            // meshopt_optimizeMeshlet( &meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset],
            //     meshlet.triangle_count, meshlet.vertex_count );

            // meshopt_Bounds bounds =
            //     meshopt_computeMeshletBounds( &meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset],
            //         meshlet.triangle_count, &pMesh.vertices[0].pos, meshlet.vertex_count, sizeof( PNodel::Vertex ) );

            PG_ASSERT( numVerts == meshlet.vertex_offset );
            pgMeshlet.vertexOffset  = meshlet.vertex_offset;
            pgMeshlet.triOffset     = numTris;
            pgMeshlet.vertexCount   = meshlet.vertex_count;
            pgMeshlet.triangleCount = meshlet.triangle_count;

            numVerts += pgMeshlet.vertexCount;
            numTris += pgMeshlet.triangleCount;
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
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const meshopt_Meshlet& moMeshlet = meshlets[meshletIdx];
            const Meshlet& pgMeshlet         = m.meshlets[meshletIdx];
            for ( u8 localVIdx = 0; localVIdx < pgMeshlet.vertexCount; ++localVIdx )
            {
                size_t globalMOIdx       = moMeshlet.vertex_offset + localVIdx;
                size_t globalPGIdx       = pgMeshlet.vertexOffset + localVIdx;
                const PModel::Vertex& v  = pMesh.vertices[meshletVertices[globalMOIdx]];
                m.positions[globalPGIdx] = v.pos;
                m.normals[globalPGIdx]   = v.normal;
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
    size_t numMeshes;
    serializer->Read( numMeshes );
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
        serializer->Read( mesh.positions );
        serializer->Read( mesh.normals );
        serializer->Read( mesh.texCoords );
        serializer->Read( mesh.tangents );
        serializer->Read( mesh.indices );
        serializer->Read( mesh.meshlets );
    }

    UploadToGPU();
    FreeCPU();

    return true;
}

bool Model::FastfileSave( Serializer* serializer ) const
{
    SerializeName( serializer );
    serializer->Write( meshes.size() );
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
    }

    return true;
}

void Model::UploadToGPU()
{
#if USING( GPU_DATA )
    using namespace Gfx;
    FreeGPU();
    for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
    {
        Mesh& mesh                    = meshes[meshIdx];
        mesh.numVertices              = static_cast<u32>( mesh.positions.size() );
        mesh.numMeshlets              = static_cast<u32>( mesh.meshlets.size() );
        mesh.hasTexCoords             = !mesh.texCoords.empty();
        mesh.hasTangents              = !mesh.tangents.empty();
        size_t totalVertexSizeInBytes = 0;
        totalVertexSizeInBytes += mesh.positions.size() * sizeof( vec3 );
        totalVertexSizeInBytes += mesh.normals.size() * sizeof( vec3 );
        totalVertexSizeInBytes += mesh.texCoords.size() * sizeof( vec2 );
        totalVertexSizeInBytes += mesh.tangents.size() * sizeof( vec4 );
        // mesh.indexOffset = totalVertexSize;
        // totalVertexSize += mesh.indices.size() * sizeof( u8 );
        // totalVertexSize += ALIGN_UP_POW_2( mesh.indices.size(), 4 ) * sizeof( u8 );

        PG_ASSERT( mesh.indices.size() % 4 == 0 );
        size_t triBufferSize      = mesh.indices.size() * sizeof( u8 );
        size_t meshletsSize       = mesh.meshlets.size() * sizeof( Meshlet );
        size_t stagingBufferSize  = Max( totalVertexSizeInBytes, Max( triBufferSize, meshletsSize ) );
        Gfx::Buffer stagingBuffer = rg.device.NewStagingBuffer( stagingBufferSize );

        char* stagingData = stagingBuffer.Map();
        memcpy( stagingData, mesh.positions.data(), mesh.positions.size() * sizeof( vec3 ) );
        stagingData += mesh.positions.size() * sizeof( vec3 );
        memcpy( stagingData, mesh.normals.data(), mesh.normals.size() * sizeof( vec3 ) );
        stagingData += mesh.normals.size() * sizeof( vec3 );
        if ( mesh.hasTexCoords )
        {
            memcpy( stagingData, mesh.texCoords.data(), mesh.texCoords.size() * sizeof( vec2 ) );
            stagingData += mesh.texCoords.size() * sizeof( vec2 );
        }
        if ( mesh.hasTangents )
        {
            memcpy( stagingData, mesh.tangents.data(), mesh.tangents.size() * sizeof( vec4 ) );
            stagingData += mesh.tangents.size() * sizeof( vec4 );
        }
        // stagingData = stagingBuffer.Map() + mesh.indexOffset;
        // memcpy( stagingData, mesh.indices.data(), mesh.indices.size() * sizeof( u8 ) );

        BufferCreateInfo vbCreateInfo{};
        vbCreateInfo.size               = totalVertexSizeInBytes;
        vbCreateInfo.bufferUsage        = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        vbCreateInfo.addToBindlessArray = false; // since BindlessManager::AddMeshBuffers will take care of it

        BufferCreateInfo tbCreateInfo = vbCreateInfo;
        tbCreateInfo.size             = triBufferSize;

        BufferCreateInfo mbCreateInfo = vbCreateInfo;
        mbCreateInfo.size             = meshletsSize;

#if USING( ASSET_NAMES )
        mesh.vertexBuffer  = rg.device.NewBuffer( vbCreateInfo, std::string( m_name ) + "_vb_" + mesh.name );
        mesh.triBuffer     = rg.device.NewBuffer( tbCreateInfo, std::string( m_name ) + "_tb_" + mesh.name );
        mesh.meshletBuffer = rg.device.NewBuffer( mbCreateInfo, std::string( m_name ) + "_mb_" + mesh.name );
#else  // #if USING( ASSET_NAMES )
        mesh.vertexBuffer  = rg.device.NewBuffer( vbCreateInfo );
        mesh.triBuffer     = rg.device.NewBuffer( tbCreateInfo );
        mesh.meshletBuffer = rg.device.NewBuffer( mbCreateInfo );
#endif // #else // #if USING( ASSET_NAMES )
        rg.ImmediateSubmit(
            [&]( CommandBuffer& cmdBuf ) { cmdBuf.CopyBuffer( mesh.vertexBuffer, stagingBuffer, totalVertexSizeInBytes ); } );

        memcpy( stagingBuffer.Map(), mesh.indices.data(), triBufferSize );
        rg.ImmediateSubmit( [&]( CommandBuffer& cmdBuf ) { cmdBuf.CopyBuffer( mesh.triBuffer, stagingBuffer, triBufferSize ); } );

        stagingData = stagingBuffer.Map();
        memcpy( stagingData, mesh.meshlets.data(), meshletsSize );
        rg.ImmediateSubmit( [&]( CommandBuffer& cmdBuf ) { cmdBuf.CopyBuffer( mesh.meshletBuffer, stagingBuffer, meshletsSize ); } );

        stagingBuffer.Free();

        mesh.bindlessBuffersSlot = BindlessManager::AddMeshBuffers( &mesh );
    }
#endif // #if USING( GPU_DATA )
}

void Model::Free()
{
    FreeGPU();
    FreeCPU();
}

void Model::FreeCPU()
{
    for ( Mesh& mesh : meshes )
    {
        mesh.meshlets  = std::vector<Meshlet>();
        mesh.positions = std::vector<vec3>();
        mesh.normals   = std::vector<vec3>();
        mesh.texCoords = std::vector<vec2>();
        mesh.tangents  = std::vector<vec4>();
        mesh.indices   = std::vector<u8>();
    }
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
        if ( mesh.vertexBuffer )
            mesh.vertexBuffer.Free();
        if ( mesh.triBuffer )
            mesh.triBuffer.Free();
        if ( mesh.meshletBuffer )
            mesh.meshletBuffer.Free();
    }
#endif // #if USING( GPU_DATA )
}

} // namespace PG
