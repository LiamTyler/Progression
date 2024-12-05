#include "asset/types/model.hpp"
#include "asset/asset_manager.hpp"
#include "asset/pmodel.hpp"
#include "asset/types/material.hpp"
#include "core/time.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <cstring>
#include <queue>
#include <stack>

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

bool CompactMeshletIndices( u32* vertIndices, u32 vertCount, u8* triIndices, u32 triCount )
{
    // Check to see if the meshlet is already compactible
    bool anyViolations = false;
    for ( u32 localTriIdx = 0; localTriIdx < triCount; ++localTriIdx )
    {
        u8vec3 tri = { triIndices[3 * localTriIdx + 0], triIndices[3 * localTriIdx + 1], triIndices[3 * localTriIdx + 2] };
        PG_ASSERT( tri.x != tri.y && tri.x != tri.z && tri.y != tri.z );
        if ( tri.x > tri.y || tri.x > tri.z )
            tri = { tri.y, tri.z, tri.x };
        if ( tri.x > tri.y || tri.x > tri.z )
            tri = { tri.y, tri.z, tri.x };

        triIndices[3 * localTriIdx + 0] = tri.x;
        triIndices[3 * localTriIdx + 1] = tri.y;
        triIndices[3 * localTriIdx + 2] = tri.z;

        u8 diff       = Max( tri.z - tri.x, tri.y - tri.x );
        anyViolations = anyViolations || ( diff >= 32 );
    }
    if ( !anyViolations )
        return true;

    struct VertexUse
    {
        u32 index;
        u32 count; // just a shortcut for adjTris.size()
        std::vector<u8> adjTris;
    };
    VertexUse vAdjacentInfos[MAX_VERTS_PER_MESHLET];
    for ( u32 i = 0; i < vertCount; ++i )
    {
        vAdjacentInfos[i].count = 0;
        vAdjacentInfos[i].index = i;
    }

    for ( u32 localTriIdx = 0; localTriIdx < triCount; ++localTriIdx )
    {
        u8vec3 tri = { triIndices[3 * localTriIdx + 0], triIndices[3 * localTriIdx + 1], triIndices[3 * localTriIdx + 2] };

        vAdjacentInfos[tri.x].count++;
        vAdjacentInfos[tri.y].count++;
        vAdjacentInfos[tri.z].count++;
        vAdjacentInfos[tri.x].adjTris.push_back( (u8)localTriIdx );
        vAdjacentInfos[tri.y].adjTris.push_back( (u8)localTriIdx );
        vAdjacentInfos[tri.z].adjTris.push_back( (u8)localTriIdx );
    }

    std::sort(
        vAdjacentInfos, vAdjacentInfos + vertCount, []( const VertexUse& lhs, const VertexUse& rhs ) { return lhs.count > rhs.count; } );

    u8 sortedVertRemap[MAX_VERTS_PER_MESHLET];
    for ( u32 i = 0; i < vertCount; ++i )
        sortedVertRemap[vAdjacentInfos[i].index] = i;

    bool triProcessed[MAX_TRIS_PER_MESHLET] = { false };
    u8 finalVertRemap[MAX_VERTS_PER_MESHLET];
    memset( finalVertRemap, 0xFF, sizeof( finalVertRemap ) );
    u8 numNewVerts = 0;
    std::vector<u8> newTriOrder;

    auto addVertFn = [&]( u8 vertIdx, std::queue<u8>& tStack )
    {
        if ( finalVertRemap[vertIdx] != 0xFF )
            return;
        finalVertRemap[vertIdx] = numNewVerts++;

        const VertexUse& adjInfo = vAdjacentInfos[sortedVertRemap[vertIdx]];
        std::vector<u8> adjTris;
        for ( u32 i = 0; i < adjInfo.count; ++i )
        {
            u8 tIdx = adjInfo.adjTris[i];
            if ( !triProcessed[tIdx] )
            {
                adjTris.push_back( tIdx );
                triProcessed[tIdx] = true;
            }
        }

        std::sort( adjTris.begin(), adjTris.end(),
            [&]( u8 lTriIdx, u8 rTriIdx )
            {
                u8vec3 lTri = { triIndices[3 * lTriIdx + 0], triIndices[3 * lTriIdx + 1], triIndices[3 * lTriIdx + 2] };
                u32 lCount  = 0;
                lCount += ( vertIdx != lTri.x ) * ( vAdjacentInfos[sortedVertRemap[lTri.x]].count );
                lCount += ( vertIdx != lTri.y ) * ( vAdjacentInfos[sortedVertRemap[lTri.y]].count );
                lCount += ( vertIdx != lTri.z ) * ( vAdjacentInfos[sortedVertRemap[lTri.z]].count );

                u8vec3 rTri = { triIndices[3 * rTriIdx + 0], triIndices[3 * rTriIdx + 1], triIndices[3 * rTriIdx + 2] };
                u32 rCount  = 0;
                rCount += ( vertIdx != rTri.x ) * ( vAdjacentInfos[sortedVertRemap[rTri.x]].count );
                rCount += ( vertIdx != rTri.y ) * ( vAdjacentInfos[sortedVertRemap[rTri.y]].count );
                rCount += ( vertIdx != rTri.z ) * ( vAdjacentInfos[sortedVertRemap[rTri.z]].count );

                return lCount < rCount;
            } );

        for ( u32 i = 0; i < adjTris.size(); ++i )
        {
            tStack.push( adjTris[adjTris.size() - i - 1] );
        }
    };

    for ( u8 sortedVIdx = 0; sortedVIdx < vertCount; ++sortedVIdx )
    {
        const VertexUse& sortedV = vAdjacentInfos[sortedVIdx];
        if ( finalVertRemap[sortedV.index] != 0xFF )
            continue;

        std::queue<u8> tStack;
        addVertFn( sortedV.index, tStack );
        while ( !tStack.empty() )
        {
            u8 tIdx = tStack.front();
            tStack.pop();
            newTriOrder.push_back( tIdx );
            u8vec3 tri = { triIndices[3 * tIdx + 0], triIndices[3 * tIdx + 1], triIndices[3 * tIdx + 2] };
            if ( vAdjacentInfos[sortedVertRemap[tri.x]].count > vAdjacentInfos[sortedVertRemap[tri.z]].count )
                std::swap( tri.x, tri.z );
            if ( vAdjacentInfos[sortedVertRemap[tri.x]].count > vAdjacentInfos[sortedVertRemap[tri.y]].count )
                std::swap( tri.x, tri.y );
            if ( vAdjacentInfos[sortedVertRemap[tri.y]].count > vAdjacentInfos[sortedVertRemap[tri.z]].count )
                std::swap( tri.y, tri.z );

            addVertFn( tri.x, tStack );
            addVertFn( tri.y, tStack );
            addVertFn( tri.z, tStack );
        }
    }
    PG_ASSERT( numNewVerts == vertCount );
    PG_ASSERT( newTriOrder.size() == triCount );

    u32 newVerts[MAX_VERTS_PER_MESHLET];
    for ( u32 i = 0; i < vertCount; ++i )
    {
        newVerts[finalVertRemap[i]] = vertIndices[i];
    }
    memcpy( vertIndices, newVerts, vertCount * sizeof( u32 ) );

    u8vec3 newTris[MAX_TRIS_PER_MESHLET];
    for ( u32 i = 0; i < triCount; ++i )
    {
        u8 tIdx      = newTriOrder[i];
        u8vec3 tri   = { triIndices[3 * tIdx + 0], triIndices[3 * tIdx + 1], triIndices[3 * tIdx + 2] };
        newTris[i].x = finalVertRemap[tri.x];
        newTris[i].y = finalVertRemap[tri.y];
        newTris[i].z = finalVertRemap[tri.z];
    }
    for ( u32 i = 0; i < triCount; ++i )
    {
        u8vec3 tri = newTris[i];
        if ( tri.x > tri.y || tri.x > tri.z )
            tri = { tri.y, tri.z, tri.x };
        if ( tri.x > tri.y || tri.x > tri.z )
            tri = { tri.y, tri.z, tri.x };

        PG_ASSERT( tri.x != tri.y && tri.x != tri.z && tri.y != tri.z );
        triIndices[3 * i + 0] = tri.x;
        triIndices[3 * i + 1] = tri.y;
        triIndices[3 * i + 2] = tri.z;
    }

    anyViolations = false;
    for ( u32 localTriIdx = 0; localTriIdx < triCount; ++localTriIdx )
    {
        u8vec3 tri = { triIndices[3 * localTriIdx + 0], triIndices[3 * localTriIdx + 1], triIndices[3 * localTriIdx + 2] };
        triIndices[3 * localTriIdx + 0] = tri.x;
        triIndices[3 * localTriIdx + 1] = tri.y;
        triIndices[3 * localTriIdx + 2] = tri.z;

        u8 diff       = Max( tri.z - tri.x, tri.y - tri.x );
        anyViolations = anyViolations || ( diff >= 32 );
    }

    return !anyViolations;
}

struct MeshHelperData
{
    std::vector<u32> meshletVertices;
};

bool Model::Load( const BaseAssetCreateInfo* baseInfo )
{
#if USING( CONVERTER )
    PG_ASSERT( baseInfo );
    const ModelCreateInfo* createInfo = (const ModelCreateInfo*)baseInfo;
    SetName( createInfo->name );

    PModel pmodel;
    if ( !pmodel.Load( GetAbsPath_ModelFilename( createInfo->filename ) ) )
        return false;

    AABB modelAABB = {};
    if ( pmodel.GetLoadedVersionNum() < PModelVersionNum::MODEL_AABB )
        pmodel.CalculateAABB();
    modelAABB.min = pmodel.aabbMin;
    modelAABB.max = pmodel.aabbMax;

    if ( createInfo->centerModel )
    {
        vec3 center = modelAABB.Center();
        for ( PModel::Mesh& pMesh : pmodel.meshes )
        {
            for ( PModel::Vertex& v : pMesh.vertices )
                v.pos -= center;
        }
        modelAABB.min -= center;
        modelAABB.max -= center;
    }

    u32 numMeshes = static_cast<u32>( pmodel.meshes.size() );
    meshes.resize( numMeshes );
    meshAABBs.resize( numMeshes );
    u64 totalVertsBefore = 0;
    u64 totalVertsAfter  = 0;
    std::vector<std::vector<u32>> allMeshletVertices( numMeshes );
    std::vector<std::vector<AABB>> allMeshletAABBs( numMeshes );
    vec3 largestMeshletExtents{ 0 };
    for ( u32 meshIdx = 0; meshIdx < numMeshes; ++meshIdx )
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

        totalVertsBefore += pMesh.vertices.size();

        size_t maxMeshlets = meshopt_buildMeshletsBound( pMesh.indices.size(), MAX_VERTS_PER_MESHLET, MAX_TRIS_PER_MESHLET );
        std::vector<meshopt_Meshlet> meshlets( maxMeshlets );
        std::vector<u32>& meshletVertices = allMeshletVertices[meshIdx];
        meshletVertices.resize( maxMeshlets * MAX_VERTS_PER_MESHLET );
        std::vector<u8> meshletTris( maxMeshlets * MAX_TRIS_PER_MESHLET * 3 );

        constexpr float CONE_WEIGHT = 0.5f;
        size_t meshletCount = meshopt_buildMeshlets( meshlets.data(), meshletVertices.data(), meshletTris.data(), pMesh.indices.data(),
            pMesh.indices.size(), &pMesh.vertices[0].pos.x, pMesh.vertices.size(), sizeof( PModel::Vertex ), MAX_VERTS_PER_MESHLET,
            MAX_TRIS_PER_MESHLET, CONE_WEIGHT );

        u32 numTris = 0;
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const meshopt_Meshlet& meshlet = meshlets[meshletIdx];
            numTris += meshlet.triangle_count;
        }
        m.indices.reserve( 4 * numTris );

        std::vector<AABB>& meshletAABBs = allMeshletAABBs[meshIdx];
        meshletAABBs.resize( meshletCount );
        m.meshlets.resize( meshletCount );
        m.meshletCullDatas.resize( meshletCount );
        u32 numMeshletVerts       = 0;
        u32 paddedTriOffset       = 0;
        u32 numCompactTriMeshlets = 0;
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const meshopt_Meshlet& meshlet = meshlets[meshletIdx];
            GpuData::Meshlet& pgMeshlet    = m.meshlets[meshletIdx];
            meshopt_optimizeMeshlet( &meshletVertices[meshlet.vertex_offset], &meshletTris[meshlet.triangle_offset], meshlet.triangle_count,
                meshlet.vertex_count );

            bool isCompact = CompactMeshletIndices( meshletVertices.data() + meshlet.vertex_offset, meshlet.vertex_count,
                meshletTris.data() + meshlet.triangle_offset, meshlet.triangle_count );

            pgMeshlet.flags = 0;
            if ( isCompact )
            {
                numCompactTriMeshlets += isCompact;
                pgMeshlet.flags |= ( 1u << MESHLET_COMPACT_INDICES_BIT );
            }

            PG_ASSERT( numMeshletVerts == meshlet.vertex_offset );
            pgMeshlet.vertexOffset  = meshlet.vertex_offset;
            pgMeshlet.triOffset     = paddedTriOffset;
            pgMeshlet.vertexCount   = meshlet.vertex_count;
            pgMeshlet.triangleCount = meshlet.triangle_count;

            totalVertsAfter += meshlet.vertex_count;
            numMeshletVerts += meshlet.vertex_count;
            u32 bytesForIndices = meshlet.triangle_count * ( isCompact ? 2 : 4 );
            m.indices.resize( m.indices.size() + bytesForIndices );
            paddedTriOffset += bytesForIndices / 2; // tris are accessed from a uint16_t array in shader

            meshopt_Bounds bounds =
                meshopt_computeMeshletBounds( &meshletVertices[meshlet.vertex_offset], &meshletTris[meshlet.triangle_offset],
                    meshlet.triangle_count, &pMesh.vertices[0].pos.x, meshlet.vertex_count, sizeof( PModel::Vertex ) );

            AABB& meshletAABB = meshletAABBs[meshletIdx];
            meshletAABB       = {};
            for ( u32 mvIdx = 0; mvIdx < meshlet.vertex_count; ++mvIdx )
            {
                u32 globalVIdx = meshletVertices[meshlet.vertex_offset + mvIdx];
                vec3 p         = pMesh.vertices[globalVIdx].pos;
                meshletAABB.Encompass( p );
            }
            largestMeshletExtents = Max( largestMeshletExtents, meshletAABB.Extent() );

            GpuData::PackedMeshletCullData& cullData = m.meshletCullDatas[meshletIdx];
            cullData.position                        = vec3( bounds.center[0], bounds.center[1], bounds.center[2] );
            cullData.radius                          = bounds.radius;
            cullData.coneAxis                        = vec3( bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2] );
            cullData.coneCutoff                      = bounds.cone_cutoff;
            cullData.coneApex                        = vec3( bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2] );

            if ( isCompact )
            {
                for ( u32 localTriIdx = 0; localTriIdx < meshlet.triangle_count; ++localTriIdx )
                {
                    size_t globalMOIdx = meshlet.triangle_offset + 3 * localTriIdx;
                    size_t globalPGIdx = 2 * pgMeshlet.triOffset + 2 * localTriIdx;
                    u8vec3 tri         = { meshletTris[globalMOIdx + 0], meshletTris[globalMOIdx + 1], meshletTris[globalMOIdx + 2] };
                    u16 packed         = 0;
                    u8 yDiff           = tri.y - tri.x;
                    u8 zDiff           = tri.z - tri.x;
                    PG_ASSERT( tri.y > tri.x && tri.z > tri.x );
                    PG_ASSERT( tri.x < 64 && yDiff < 32 && zDiff < 32 );
                    packed |= tri.x;
                    packed |= yDiff << 6;
                    packed |= zDiff << 11;
                    m.indices[globalPGIdx + 0] = packed & 0xFF;
                    m.indices[globalPGIdx + 1] = ( packed >> 8 ) & 0xFF;
                }
            }
            else
            {
                for ( u32 localTriIdx = 0; localTriIdx < meshlet.triangle_count; ++localTriIdx )
                {
                    size_t globalMOIdx = meshlet.triangle_offset + 3 * localTriIdx;
                    size_t globalPGIdx = 2 * pgMeshlet.triOffset + 4 * localTriIdx;
                    u8vec3 tri         = { meshletTris[globalMOIdx + 0], meshletTris[globalMOIdx + 1], meshletTris[globalMOIdx + 2] };
                    m.indices[globalPGIdx + 0] = tri.x;
                    m.indices[globalPGIdx + 1] = tri.y;
                    m.indices[globalPGIdx + 2] = tri.z;
                    m.indices[globalPGIdx + 3] = 0;
                }
            }
        }

        if ( numCompactTriMeshlets != meshletCount )
        {
            LOG_WARN( "Compactible Meshlets: %u/%u = %.2f%%", numCompactTriMeshlets, meshletCount,
                100.0f * numCompactTriMeshlets / (float)meshletCount );
        }
    }

#if PACKED_VERTS
    // Quantization strategy described here: https://gpuopen.com/learn/mesh_shaders/mesh_shaders-meshlet_compression/
    const u32 TARGET_BITS               = 16;
    const vec3 globalMin                = modelAABB.min;
    const vec3 globalDelta              = modelAABB.max - modelAABB.min;
    const vec3 meshletQuantizationStep  = largestMeshletExtents / float( ( 1 << TARGET_BITS ) - 1 );
    const vec3 globalQuantizationStates = vec3( uvec3( globalDelta / meshletQuantizationStep ) );
    const vec3 effectiveBits            = glm::log2( globalQuantizationStates );
    const vec3 quantizationFactor       = ( globalQuantizationStates - vec3( 1 ) ) / globalDelta;
    LOG( "Effective Quantization Bits: %f %f %f", effectiveBits.x, effectiveBits.y, effectiveBits.z );

    positionDequantizationInfo.factor    = globalDelta / ( globalQuantizationStates - vec3( 1 ) );
    positionDequantizationInfo.globalMin = globalMin;
#endif // #if PACKED_VERTS

    uvec3 maxQuantizedValue = uvec3( 0 );
    for ( u32 meshIdx = 0; meshIdx < numMeshes; ++meshIdx )
    {
        Mesh& m                   = meshes[meshIdx];
        const PModel::Mesh& pMesh = pmodel.meshes[meshIdx];
        u32 numMeshletVerts       = 0;
        for ( size_t meshletIdx = 0; meshletIdx < m.meshlets.size(); ++meshletIdx )
            numMeshletVerts += m.meshlets[meshletIdx].vertexCount;

        u32 numZeroNormals  = 0;
        u32 numZeroTangents = 0;
        AABB& meshAABB      = meshAABBs[meshIdx];

#if VERT_INDICES
        u32 numMeshVerts = static_cast<u32>( pMesh.vertices.size() );
        m.positions.resize( numMeshVerts );
        m.normals.resize( numMeshVerts );
        if ( pMesh.numUVChannels > 0 )
            m.texCoords.resize( numMeshVerts );
        if ( pMesh.hasTangents )
            m.tangents.resize( numMeshVerts );
        for ( u32 mvIdx = 0; mvIdx < numMeshVerts; ++mvIdx )
        {
            const PModel::Vertex& v = pMesh.vertices[mvIdx];
            m.positions[mvIdx]      = v.pos;
            meshAABB.Encompass( v.pos );
            m.normals[mvIdx] = v.normal;
            if ( Length( v.normal ) <= 0.00001f )
                ++numZeroNormals;

            if ( pMesh.numUVChannels > 0 )
            {
                m.texCoords[mvIdx] = v.uvs[0];
                if ( createInfo->flipTexCoordsVertically )
                    m.texCoords[mvIdx].y = 1.0f - v.uvs[0].y;
            }
            if ( pMesh.hasTangents )
            {
                vec3 tangent      = v.tangent;
                vec3 bitangent    = v.bitangent;
                vec3 tNormal      = Cross( tangent, bitangent );
                float bSign       = Dot( v.normal, tNormal ) >= 0.0f ? 1.0f : -1.0f;
                vec4 packed       = vec4( tangent, bSign );
                m.tangents[mvIdx] = packed;
                if ( Length( v.tangent ) <= 0.00001f )
                    ++numZeroTangents;
            }
        }

        m.vertIndices.resize( numMeshletVerts );
        u32 num8VI  = 0;
        u32 num16VI = 0;
        for ( size_t meshletIdx = 0; meshletIdx < meshletCount; ++meshletIdx )
        {
            const GpuData::Meshlet& pgMeshlet = m.meshlets[meshletIdx];
            u32 minVI                         = UINT_MAX;
            u32 maxVI                         = 0;
            for ( u32 localVIdx = 0; localVIdx < pgMeshlet.vertexCount; ++localVIdx )
            {
                minVI                                             = Min( minVI, meshletVertices[pgMeshlet.vertexOffset + localVIdx] );
                maxVI                                             = Max( maxVI, meshletVertices[pgMeshlet.vertexOffset + localVIdx] );
                m.vertIndices[pgMeshlet.vertexOffset + localVIdx] = meshletVertices[pgMeshlet.vertexOffset + localVIdx];
            }
            u32 d = maxVI - minVI;
            if ( d < 256 )
                ++num8VI;
            else if ( d < 65536 )
                ++num16VI;
        }
        LOG( "Num 8 VI: %u, 16 VI: %u. Total: %u", num8VI, num16VI, meshletCount );
#else // #if VERT_INDICES
        m.packedPositions.resize( numMeshletVerts );
        m.normals.resize( numMeshletVerts );
        if ( pMesh.numUVChannels > 0 )
            m.texCoords.resize( numMeshletVerts );
        if ( pMesh.hasTangents )
            m.tangents.resize( numMeshletVerts );

        const std::vector<AABB>& meshletAABBs   = allMeshletAABBs[meshIdx];
        const std::vector<u32>& meshletVertices = allMeshletVertices[meshIdx];
        for ( size_t meshletIdx = 0; meshletIdx < m.meshlets.size(); ++meshletIdx )
        {
            GpuData::Meshlet& pgMeshlet = m.meshlets[meshletIdx];
#if PACKED_VERTS
            const uvec3 quantizedMeshletOffset = ( meshletAABBs[meshletIdx].min - globalMin ) * quantizationFactor + 0.5f;
            pgMeshlet.quantizedMeshletOffset   = quantizedMeshletOffset;
#endif // #if PACKED_VERTS

            for ( u32 localVIdx = 0; localVIdx < pgMeshlet.vertexCount; ++localVIdx )
            {
                size_t globalVIdx       = pgMeshlet.vertexOffset + localVIdx;
                const PModel::Vertex& v = pMesh.vertices[meshletVertices[globalVIdx]];
                meshAABB.Encompass( v.pos );

#if PACKED_VERTS
                uvec3 globalQuantizedValue    = ( v.pos - globalMin ) * quantizationFactor + 0.5f;
                uvec3 localQuantizedValue     = globalQuantizedValue - quantizedMeshletOffset;
                maxQuantizedValue             = glm::max( maxQuantizedValue, localQuantizedValue );
                m.packedPositions[globalVIdx] = u16vec3( localQuantizedValue );
#else  // #if PACKED_VERTS
                m.packedPositions[globalVIdx] = v.pos;
#endif // #else // #if PACKED_VERTS

                m.normals[globalVIdx] = v.normal;
                if ( Length( v.normal ) <= 0.00001f )
                    ++numZeroNormals;

                if ( pMesh.numUVChannels > 0 )
                {
                    m.texCoords[globalVIdx] = v.uvs[0];
                    if ( createInfo->flipTexCoordsVertically )
                        m.texCoords[globalVIdx].y = 1.0f - v.uvs[0].y;
                }
                if ( pMesh.hasTangents )
                {
                    vec3 tangent           = v.tangent;
                    vec3 bitangent         = v.bitangent;
                    vec3 tNormal           = Cross( tangent, bitangent );
                    float bSign            = Dot( v.normal, tNormal ) > 0.0f ? 1.0f : -1.0f;
                    vec4 packed            = vec4( tangent, bSign );
                    m.tangents[globalVIdx] = packed;
                    if ( Length( v.tangent ) <= 0.00001f )
                        ++numZeroTangents;
                }
            }
        }
#endif // #else // #if VERT_INDICES
        if ( numZeroNormals )
            LOG_WARN( "Mesh[%u] '%s' inside of model '%s' has %u normals of length 0", meshIdx, pMesh.name.c_str(),
                createInfo->name.c_str(), numZeroNormals );
        if ( numZeroTangents )
            LOG_WARN( "Mesh[%u] '%s' inside of model '%s' has %u tangents of length 0", meshIdx, pMesh.name.c_str(),
                createInfo->name.c_str(), numZeroTangents );
    }
    PG_ASSERT( !createInfo->recalculateNormals, "Not implemented with meshlets yet" );

    LOG( "Model verts: %zu, Meshlet verts: %zu. %.2f%% increase", totalVertsBefore, totalVertsAfter,
        ( totalVertsAfter / (double)totalVertsBefore - 1.0 ) * 100.0 );

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
        serializer->Read( mesh.packedPositions );
        serializer->Read( mesh.normals );
        serializer->Read( mesh.texCoords );
        serializer->Read( mesh.tangents );
        serializer->Read( mesh.indices );
#if VERT_INDICES
        serializer->Read( mesh.vertIndices );
#endif // #if VERT_INDICES
        serializer->Read( mesh.meshlets );
        serializer->Read( mesh.meshletCullDatas );
#else // #if !USING( GPU_DATA )
        u64 numPos          = serializer->Read<u64>();
        const void* posData = serializer->GetData();
        u64 posDataSize     = numPos * ( PACKED_VERTS ? sizeof( u16vec3 ) : sizeof( vec3 ) );
        serializer->Skip( posDataSize );

        u64 numNormal          = serializer->Read<u64>();
        const void* normalData = serializer->GetData();
        serializer->Skip( numNormal * sizeof( vec3 ) );

        u64 numTexCoords   = serializer->Read<u64>();
        const void* uvData = serializer->GetData();
        serializer->Skip( numTexCoords * sizeof( vec2 ) );

        u64 numTan          = serializer->Read<u64>();
        const void* tanData = serializer->GetData();
        serializer->Skip( numTan * sizeof( vec4 ) );

        u64 numIndices        = serializer->Read<u64>();
        const void* indexData = serializer->GetData();
        serializer->Skip( numIndices * sizeof( u8 ) );

#if VERT_INDICES
        u64 numVertIndices        = serializer->Read<u64>();
        const void* vertIndexData = serializer->GetData();
        serializer->Skip( numVertIndices * sizeof( u32 ) );
#endif // #if VERT_INDICES

        u64 numMeshlets         = serializer->Read<u64>();
        const void* meshletData = serializer->GetData();
        serializer->Skip( numMeshlets * sizeof( GpuData::Meshlet ) );

        const void* meshletCullData = serializer->GetData();
        serializer->Skip( numMeshlets * sizeof( GpuData::PackedMeshletCullData ) );

        mesh.numVertices  = static_cast<u32>( numPos );
        mesh.numMeshlets  = static_cast<u32>( numMeshlets );
        mesh.hasTexCoords = numTexCoords != 0;
        mesh.hasTangents  = numTan != 0;

        size_t totalVertexSizeInBytes = 0;
        totalVertexSizeInBytes += ROUND_UP_TO_MULT( posDataSize, 4 );
        totalVertexSizeInBytes += numNormal * sizeof( vec3 );
        totalVertexSizeInBytes += numTexCoords * sizeof( vec2 );
        totalVertexSizeInBytes += numTan * sizeof( vec4 );

        BufferCreateInfo vbCreateInfo{};
        vbCreateInfo.size               = totalVertexSizeInBytes;
        vbCreateInfo.bufferUsage        = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        vbCreateInfo.addToBindlessArray = false; // since BindlessManager::AddMeshBuffers will take care of it

#if VERT_INDICES
        BufferCreateInfo vibCreateInfo = vbCreateInfo;
        vibCreateInfo.size             = numVertIndices * sizeof( u32 );
#endif // #if VERT_INDICES

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
#if VERT_INDICES
        mesh.buffers[Mesh::VERTEX_INDICES_BUFFER] = rg.device.NewBuffer( vibCreateInfo, std::string( m_name ) + mesh.name + "_vib" );
#endif // #if VERT_INDICES
#else  // #if USING( ASSET_NAMES )
        mesh.buffers[Mesh::VERTEX_BUFFER]    = rg.device.NewBuffer( vbCreateInfo );
        mesh.buffers[Mesh::TRI_BUFFER]       = rg.device.NewBuffer( tbCreateInfo );
        mesh.buffers[Mesh::MESHLET_BUFFER]   = rg.device.NewBuffer( mbCreateInfo );
        mesh.buffers[Mesh::CULL_DATA_BUFFER] = rg.device.NewBuffer( cbCreateInfo );
#if VERT_INDICES
        mesh.buffers[Mesh::VERTEX_INDICES_BUFFER] = rg.device.NewBuffer( vibCreateInfo );
#endif // #if VERT_INDICES
#endif // #else // #if USING( ASSET_NAMES )
        mesh.bindlessBuffersSlot = BindlessManager::AddMeshBuffers( &mesh );

        size_t offset = 0;
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], posData, posDataSize, offset );
        offset += ROUND_UP_TO_MULT( posDataSize, 4 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], normalData, numNormal * sizeof( vec3 ), offset );
        offset += numNormal * sizeof( vec3 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], uvData, numTexCoords * sizeof( vec2 ), offset );
        offset += numTexCoords * sizeof( vec2 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], tanData, numTan * sizeof( vec4 ), offset );
        offset += numTan * sizeof( vec4 );

        rg.device.AddUploadRequest( mesh.buffers[Mesh::TRI_BUFFER], indexData, tbCreateInfo.size );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::MESHLET_BUFFER], meshletData, mbCreateInfo.size );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::CULL_DATA_BUFFER], meshletCullData, cbCreateInfo.size );
#if VERT_INDICES
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_INDICES_BUFFER], vertIndexData, vibCreateInfo.size );
#endif // #if VERT_INDICES
#endif // #else // #if !USING( GPU_DATA )
    }
    serializer->Read( positionDequantizationInfo );

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
        serializer->Write( mesh.packedPositions );
        serializer->Write( mesh.normals );
        serializer->Write( mesh.texCoords );
        serializer->Write( mesh.tangents );
        serializer->Write( mesh.indices );
#if VERT_INDICES
        serializer->Write( mesh.vertIndices );
#endif // #if VERT_INDICES
        serializer->Write( mesh.meshlets );
        serializer->Write( mesh.meshletCullDatas.data(), mesh.meshletCullDatas.size() * sizeof( GpuData::PackedMeshletCullData ) );
    }
    serializer->Write( positionDequantizationInfo );
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
        mesh.meshletCullDatas = {};
        mesh.meshlets         = {};
        mesh.packedPositions  = {};
        mesh.normals          = {};
        mesh.texCoords        = {};
        mesh.tangents         = {};
        mesh.indices          = {};
#if VERT_INDICES
        mesh.vertIndices = {};
#endif // #if VERT_INDICES
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
