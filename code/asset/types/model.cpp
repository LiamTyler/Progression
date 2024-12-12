#include "asset/types/model.hpp"
#include "asset/asset_manager.hpp"
#include "asset/pmodel.hpp"
#include "asset/types/material.hpp"
#include "core/cpu_profiling.hpp"
#include "core/time.hpp"
#include "shared/assert.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/serializer.hpp"
#include <cstring>
#include <queue>

#if USING( CONVERTER )
#include "data_structures/queue_static.hpp"
#include "meshoptimizer/src/meshoptimizer.h"
#include "shared/oct_encoding.hpp"
#endif // #if USING( CONVERTER )
#if USING( GPU_DATA )
using namespace PG::Gfx;
#include "renderer/graphics_api/buffer.hpp"
#include "renderer/r_bindless_manager.hpp"
#include "renderer/r_globals.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

#if USING( CONVERTER )
std::string GetAbsPath_ModelFilename( const std::string& filename ) { return PG_ASSET_DIR + filename; }

bool CompactMeshletIndices( u32* vertIndices, u32 vertCount, u8* triIndices, u32 triCount )
{
    // Check to see if the meshlet is already compactible
    bool anyViolations = false;
    for ( u32 localTriIdx = 0; localTriIdx < triCount; ++localTriIdx )
    {
        u8vec3 tri = { triIndices[3 * localTriIdx + 0], triIndices[3 * localTriIdx + 1], triIndices[3 * localTriIdx + 2] };
        PG_DBG_ASSERT( tri.x != tri.y && tri.x != tri.z && tri.y != tri.z );
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

    auto addVertFn = [&]( u8 vertIdx, StaticQueue<u8, MAX_TRIS_PER_MESHLET>& tQueue )
    {
        if ( finalVertRemap[vertIdx] != 0xFF )
            return;
        finalVertRemap[vertIdx] = numNewVerts++;

        const VertexUse& adjInfo = vAdjacentInfos[sortedVertRemap[vertIdx]];
        u8 adjTris[MAX_TRIS_PER_MESHLET];
        u32 adjTrisCount = 0;
        for ( u32 i = 0; i < adjInfo.count; ++i )
        {
            u8 tIdx = adjInfo.adjTris[i];
            if ( !triProcessed[tIdx] )
            {
                adjTris[adjTrisCount++] = tIdx;
                triProcessed[tIdx]      = true;
            }
        }

        std::sort( adjTris, adjTris + adjTrisCount,
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

        for ( u32 i = 0; i < adjTrisCount; ++i )
        {
            tQueue.push( adjTris[adjTrisCount - i - 1] );
        }
    };

    u8 newTriOrder[MAX_TRIS_PER_MESHLET];
    u32 newTriOrderCount = 0;
    for ( u8 sortedVIdx = 0; sortedVIdx < vertCount; ++sortedVIdx )
    {
        const VertexUse& sortedV = vAdjacentInfos[sortedVIdx];
        if ( finalVertRemap[sortedV.index] != 0xFF )
            continue;

        StaticQueue<u8, MAX_TRIS_PER_MESHLET> tQueue;
        addVertFn( sortedV.index, tQueue );
        while ( !tQueue.empty() )
        {
            u8 tIdx = tQueue.front();
            tQueue.pop();
            newTriOrder[newTriOrderCount++] = tIdx;
            u8vec3 tri                      = { triIndices[3 * tIdx + 0], triIndices[3 * tIdx + 1], triIndices[3 * tIdx + 2] };
            if ( vAdjacentInfos[sortedVertRemap[tri.x]].count > vAdjacentInfos[sortedVertRemap[tri.z]].count )
                std::swap( tri.x, tri.z );
            if ( vAdjacentInfos[sortedVertRemap[tri.x]].count > vAdjacentInfos[sortedVertRemap[tri.y]].count )
                std::swap( tri.x, tri.y );
            if ( vAdjacentInfos[sortedVertRemap[tri.y]].count > vAdjacentInfos[sortedVertRemap[tri.z]].count )
                std::swap( tri.y, tri.z );

            addVertFn( tri.x, tQueue );
            addVertFn( tri.y, tQueue );
            addVertFn( tri.z, tQueue );
        }
    }
    PG_DBG_ASSERT( numNewVerts == vertCount );
    PG_DBG_ASSERT( newTriOrderCount == triCount );

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

        PG_DBG_ASSERT( tri.x != tri.y && tri.x != tri.z && tri.y != tri.z );
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

struct MeshBuildData
{
    std::vector<meshopt_Meshlet> moMeshlets;
    std::vector<u32> meshletVertices;
    std::vector<u8> meshletTris;
    std::vector<AABB> meshletAABBs;
    u32 numMeshletVerts        = 0;
    u32 numMeshletTris         = 0;
    u32 numMeshlets            = 0;
    u32 nonCompactibleMeshlets = 0;
};

static GpuData::Meshlet MoToPGMeshlet( u32 vertCount, u32 triCount, u32& numTotalVerts, u32& numTotalTris )
{
    GpuData::Meshlet pgMeshlet;
    pgMeshlet.vertexOffset   = numTotalVerts;
    pgMeshlet.triangleOffset = numTotalTris;
    pgMeshlet.vertexCount    = vertCount;
    pgMeshlet.triangleCount  = triCount;
    numTotalVerts += vertCount;
    numTotalTris += triCount;

    return pgMeshlet;
}

struct Submeshlet
{
    u32 vertIndices[MAX_VERTS_PER_MESHLET];
    u8 triIndices[3 * MAX_TRIS_PER_MESHLET];
    u32 vertCount = 0;
    u32 triCount  = 0;
};

static void FindCompactibleSubmeshlet( const u32* vertIndices, const u8* triIndices, u32 triOffset, u32 triCount, Submeshlet& submeshlet )
{
    submeshlet = {};
    u8 indexRemap[MAX_TRIS_PER_MESHLET];
    memset( indexRemap, 0xFF, sizeof( indexRemap ) );
    for ( u32 i = triOffset; i < triCount; ++i )
    {
        u8vec3 tri = { triIndices[3 * i + 0], triIndices[3 * i + 1], triIndices[3 * i + 2] };

        // see if the remapped tri would be compactible
        u32 tmpVertCount = submeshlet.vertCount;
        for ( int j = 0; j < 3; ++j )
        {
            u8 index = tri[j];
            if ( indexRemap[index] == 0xFF )
            {
                submeshlet.vertIndices[tmpVertCount] = vertIndices[index];
                indexRemap[index]                    = tmpVertCount++;
            }
        }

        u8vec3 remappedTri = { indexRemap[tri.x], indexRemap[tri.y], indexRemap[tri.z] };
        if ( remappedTri.x > remappedTri.y || remappedTri.x > remappedTri.z )
            remappedTri = { remappedTri.y, remappedTri.z, remappedTri.x };
        if ( remappedTri.x > remappedTri.y || remappedTri.x > remappedTri.z )
            remappedTri = { remappedTri.y, remappedTri.z, remappedTri.x };
        u8 diff = Max( remappedTri.z - remappedTri.x, remappedTri.y - remappedTri.x );
        if ( diff >= 32 )
            return;

        submeshlet.vertCount                               = tmpVertCount;
        submeshlet.triIndices[3 * submeshlet.triCount + 0] = remappedTri.x;
        submeshlet.triIndices[3 * submeshlet.triCount + 1] = remappedTri.y;
        submeshlet.triIndices[3 * submeshlet.triCount + 2] = remappedTri.z;
        submeshlet.triCount++;
    }
}

static void CompactMeshlets( Mesh& m, MeshBuildData& buildData )
{
    PGP_MANUAL_ZONEN( __CompactMeshlets, "CompactMeshlets" );
    std::vector<u8> subMeshletsPostCompaction( buildData.numMeshlets );
    int numBlocks                  = ROUND_UP_DIV( buildData.numMeshlets, 64 );
    std::atomic<int> extraMeshlets = 0;
#pragma omp parallel for
    for ( int blockIdx = 0; blockIdx < numBlocks; ++blockIdx )
    {
        u32 meshletOffset           = 64 * (u32)blockIdx;
        int blockLocalExtraMeshlets = 0;
        for ( u32 localMeshletIdx = 0; localMeshletIdx < 64; ++localMeshletIdx )
        {
            u32 meshletIdx = meshletOffset + localMeshletIdx;
            if ( meshletIdx >= buildData.numMeshlets )
                break;

            const meshopt_Meshlet& meshlet = buildData.moMeshlets[meshletIdx];
            meshopt_optimizeMeshlet( &buildData.meshletVertices[meshlet.vertex_offset], &buildData.meshletTris[meshlet.triangle_offset],
                meshlet.triangle_count, meshlet.vertex_count );
            bool isCompact = CompactMeshletIndices( buildData.meshletVertices.data() + meshlet.vertex_offset, meshlet.vertex_count,
                buildData.meshletTris.data() + meshlet.triangle_offset, meshlet.triangle_count );

            subMeshletsPostCompaction[meshletIdx] = 1 + !isCompact;
            blockLocalExtraMeshlets += !isCompact;
        }
        extraMeshlets += blockLocalExtraMeshlets;
    }
    PGP_MANUAL_ZONE_END( __CompactMeshlets );

    u32 numMeshletVerts = 0;
    u32 numMeshletTris  = 0;
    PGP_MANUAL_ZONEN( __CompactionFixup, "CompactionFixup" );
    u32 estimatedNewMeshlets = buildData.numMeshlets + 3 * extraMeshlets.load();
    m.meshlets.reserve( estimatedNewMeshlets );
    std::vector<u32> newMeshletVertices;
    newMeshletVertices.reserve( estimatedNewMeshlets * MAX_VERTS_PER_MESHLET );
    std::vector<u8> newMeshletTris;
    newMeshletTris.reserve( estimatedNewMeshlets * 3 * MAX_TRIS_PER_MESHLET );
    for ( u32 moIdx = 0; moIdx < buildData.numMeshlets; ++moIdx )
    {
        u8 numMeshlets             = subMeshletsPostCompaction[moIdx];
        meshopt_Meshlet& moMeshlet = buildData.moMeshlets[moIdx];

        if ( numMeshlets == 1 ) [[likely]]
        {
            m.meshlets.push_back( MoToPGMeshlet( moMeshlet.vertex_count, moMeshlet.triangle_count, numMeshletVerts, numMeshletTris ) );
            auto vertIter = buildData.meshletVertices.begin() + moMeshlet.vertex_offset;
            newMeshletVertices.insert( newMeshletVertices.end(), vertIter, vertIter + moMeshlet.vertex_count );
            auto triIter = buildData.meshletTris.begin() + moMeshlet.triangle_offset;
            newMeshletTris.insert( newMeshletTris.end(), triIter, triIter + 3 * moMeshlet.triangle_count );
        }
        else [[unlikely]]
        {
            u32* vertIndices  = buildData.meshletVertices.data() + moMeshlet.vertex_offset;
            u8* triIndices    = buildData.meshletTris.data() + moMeshlet.triangle_offset;
            u32 processedTris = 0;
            while ( processedTris < moMeshlet.triangle_count )
            {
                Submeshlet submeshlet;
                FindCompactibleSubmeshlet( vertIndices, triIndices, processedTris, moMeshlet.triangle_count, submeshlet );
                processedTris += submeshlet.triCount;

                newMeshletVertices.insert(
                    newMeshletVertices.end(), submeshlet.vertIndices, submeshlet.vertIndices + submeshlet.vertCount );
                newMeshletTris.insert( newMeshletTris.end(), submeshlet.triIndices, submeshlet.triIndices + 3 * submeshlet.triCount );
                m.meshlets.push_back( MoToPGMeshlet( submeshlet.vertCount, submeshlet.triCount, numMeshletVerts, numMeshletTris ) );
            }
        }
    }
    std::swap( buildData.meshletVertices, newMeshletVertices );
    std::swap( buildData.meshletTris, newMeshletTris );
    buildData.numMeshletVerts        = numMeshletVerts;
    buildData.numMeshletTris         = numMeshletTris;
    buildData.numMeshlets            = static_cast<u32>( m.meshlets.size() );
    buildData.nonCompactibleMeshlets = extraMeshlets.load();
    PGP_MANUAL_ZONE_END( __CompactionFixup );
}

static void OptimizeMeshletsWithoutCompaction( Mesh& m, MeshBuildData& buildData )
{
    PGP_ZONE_SCOPEDN( "OptimizeMeshletsWithoutCompaction" );
    int numBlocks = ROUND_UP_DIV( buildData.numMeshlets, 64 );
#pragma omp parallel for
    for ( int blockIdx = 0; blockIdx < numBlocks; ++blockIdx )
    {
        u32 meshletOffset = 64 * (u32)blockIdx;
        for ( u32 localMeshletIdx = 0; localMeshletIdx < 64; ++localMeshletIdx )
        {
            u32 meshletIdx = meshletOffset + localMeshletIdx;
            if ( meshletIdx >= buildData.numMeshlets )
                break;

            const meshopt_Meshlet& meshlet = buildData.moMeshlets[meshletIdx];
            meshopt_optimizeMeshlet( &buildData.meshletVertices[meshlet.vertex_offset], &buildData.meshletTris[meshlet.triangle_offset],
                meshlet.triangle_count, meshlet.vertex_count );
        }
    }

    u32 numMeshletVerts = 0;
    u32 numMeshletTris  = 0;
    m.meshlets.reserve( buildData.numMeshlets );
    std::vector<u8> newMeshletTris;
    newMeshletTris.reserve( m.meshlets.size() * MAX_TRIS_PER_MESHLET );
    for ( u32 moIdx = 0; moIdx < buildData.numMeshlets; ++moIdx )
    {
        meshopt_Meshlet& moMeshlet = buildData.moMeshlets[moIdx];
        PG_ASSERT( moMeshlet.vertex_offset == numMeshletVerts );
        m.meshlets.push_back( MoToPGMeshlet( moMeshlet.vertex_count, moMeshlet.triangle_count, numMeshletVerts, numMeshletTris ) );
        auto triIter = buildData.meshletTris.begin() + moMeshlet.triangle_offset;
        newMeshletTris.insert( newMeshletTris.end(), triIter, triIter + 3 * moMeshlet.triangle_count );
    }

    std::swap( buildData.meshletTris, newMeshletTris );
    buildData.numMeshletVerts = numMeshletVerts;
    buildData.numMeshletTris  = numMeshletTris;
}

#endif // #if USING( CONVERTER )

bool Model::Load( const BaseAssetCreateInfo* baseInfo )
{
    PGP_ZONE_SCOPEDN( "Model::Load" );
#if USING( CONVERTER )
    PG_ASSERT( baseInfo );
    const ModelCreateInfo* createInfo = (const ModelCreateInfo*)baseInfo;
    SetName( createInfo->name );
    PG_ASSERT( !createInfo->recalculateNormals, "Not implemented with meshlets yet" );

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
        PGP_ZONE_SCOPEDN( "CenterModel" );
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
    std::vector<MeshBuildData> meshBuildDatas( numMeshes );
    meshes.resize( numMeshes );
    meshAABBs.resize( numMeshes );
    vec3 largestMeshletExtents{ 0 };
#if PACKED_TRIS
    u32 nonCompactibleMeshlets      = 0;
    u32 totalMeshletsPreCompaction  = 0;
    u32 totalMeshletsPostCompaction = 0;
#endif // #if PACKED_TRIS
    for ( u32 meshIdx = 0; meshIdx < numMeshes; ++meshIdx )
    {
        Mesh& m                   = meshes[meshIdx];
        const PModel::Mesh& pMesh = pmodel.meshes[meshIdx];
        MeshBuildData& buildData  = meshBuildDatas[meshIdx];

        m.name     = pMesh.name;
        m.material = AssetManager::Get<Material>( pMesh.materialName );
        if ( !m.material )
        {
            LOG_ERR( "No material '%s' found", pMesh.materialName.c_str() );
            return false;
        }

        PGP_MANUAL_ZONEN( __buildMeshlets, "BuildMeshlets" );
        u64 maxMeshlets = meshopt_buildMeshletsBound( pMesh.indices.size(), MAX_VERTS_PER_MESHLET, MAX_TRIS_PER_MESHLET );
        buildData.moMeshlets.resize( maxMeshlets );
        buildData.meshletVertices.resize( maxMeshlets * MAX_VERTS_PER_MESHLET );
        buildData.meshletTris.resize( maxMeshlets * MAX_TRIS_PER_MESHLET * 3 );

        constexpr float CONE_WEIGHT = 0.5f;
        buildData.numMeshlets       = (u32)meshopt_buildMeshlets( buildData.moMeshlets.data(), buildData.meshletVertices.data(),
                  buildData.meshletTris.data(), pMesh.indices.data(), pMesh.indices.size(), &pMesh.vertices[0].pos.x, pMesh.vertices.size(),
                  sizeof( PModel::Vertex ), MAX_VERTS_PER_MESHLET, MAX_TRIS_PER_MESHLET, CONE_WEIGHT );
        PGP_MANUAL_ZONE_END( __buildMeshlets );

#if PACKED_TRIS
        totalMeshletsPreCompaction += buildData.numMeshlets;
        CompactMeshlets( m, buildData );
        nonCompactibleMeshlets += buildData.nonCompactibleMeshlets;
        totalMeshletsPostCompaction += buildData.numMeshlets;
#else  // #if PACKED_TRIS
        OptimizeMeshletsWithoutCompaction( m, buildData );
#endif // #else // #if PACKED_TRIS

        PGP_MANUAL_ZONEN( __MeshletBoundsAndTris, "MeshletBoundsAndTris" );
        m.packedTris.resize( PACKED_TRIS ? buildData.numMeshletTris : ( buildData.numMeshletTris * 3 ) );
        buildData.meshletAABBs.resize( buildData.numMeshlets );
        m.meshletCullDatas.resize( buildData.numMeshlets );

        int numBlocks = ROUND_UP_DIV( buildData.numMeshlets, 64 );
        std::mutex extentsLock;
#pragma omp parallel for
        for ( int blockIdx = 0; blockIdx < numBlocks; ++blockIdx )
        {
            u32 meshletOffset = 64 * (u32)blockIdx;
            vec3 blockLocalLargestMeshletExtents{ 0 };
            AABB blockLocalMeshAABB = {};
            for ( u32 localMeshletIdx = 0; localMeshletIdx < 64; ++localMeshletIdx )
            {
                u32 meshletIdx = meshletOffset + localMeshletIdx;
                if ( meshletIdx >= buildData.numMeshlets )
                    break;

                const GpuData::Meshlet& meshlet = m.meshlets[meshletIdx];
                meshopt_Bounds bounds           = meshopt_computeMeshletBounds( &buildData.meshletVertices[meshlet.vertexOffset],
                              &buildData.meshletTris[3 * meshlet.triangleOffset], meshlet.triangleCount, &pMesh.vertices[0].pos.x,
                              meshlet.vertexCount, sizeof( PModel::Vertex ) );

                AABB& meshletAABB = buildData.meshletAABBs[meshletIdx];
                meshletAABB       = {};
                for ( u32 mvIdx = 0; mvIdx < meshlet.vertexCount; ++mvIdx )
                {
                    u32 globalVIdx = buildData.meshletVertices[meshlet.vertexOffset + mvIdx];
                    vec3 p         = pMesh.vertices[globalVIdx].pos;
                    meshletAABB.Encompass( p );
                }
                blockLocalLargestMeshletExtents = Max( blockLocalLargestMeshletExtents, meshletAABB.Extent() );
                blockLocalMeshAABB.Encompass( meshletAABB );

                GpuData::PackedMeshletCullData& cullData = m.meshletCullDatas[meshletIdx];
                cullData.position                        = vec3( bounds.center[0], bounds.center[1], bounds.center[2] );
                cullData.radius                          = bounds.radius;
                cullData.coneAxis                        = vec3( bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2] );
                cullData.coneCutoff                      = bounds.cone_cutoff;
                cullData.coneApex                        = vec3( bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2] );

                for ( u32 localTriIdx = 0; localTriIdx < meshlet.triangleCount; ++localTriIdx )
                {
                    u64 globalMOIdx = 3 * ( meshlet.triangleOffset + localTriIdx );
                    u8vec3 tri      = { buildData.meshletTris[globalMOIdx + 0], buildData.meshletTris[globalMOIdx + 1],
                             buildData.meshletTris[globalMOIdx + 2] };

#if PACKED_TRIS
                    PG_ASSERT( tri.x < tri.y && tri.x < tri.z );

                    u16 packedTri = 0;
                    u8 yDiff      = tri.y - tri.x;
                    u8 zDiff      = tri.z - tri.x;
                    PG_ASSERT( tri.x < 64 && yDiff < 32 && zDiff < 32 );
                    packedTri |= tri.x;
                    packedTri |= yDiff << 6;
                    packedTri |= zDiff << 11;
                    m.packedTris[meshlet.triangleOffset + localTriIdx] = packedTri;
#else  // #if PACKED_TRIS
                    m.packedTris[globalMOIdx + 0] = tri.x;
                    m.packedTris[globalMOIdx + 1] = tri.y;
                    m.packedTris[globalMOIdx + 2] = tri.z;
#endif // #else // #if PACKED_TRIS
                }
            }

            extentsLock.lock();
            largestMeshletExtents = Max( largestMeshletExtents, blockLocalLargestMeshletExtents );
            meshAABBs[meshIdx].Encompass( blockLocalMeshAABB );
            extentsLock.unlock();
        }
        PGP_MANUAL_ZONE_END( __MeshletBoundsAndTris );
    }

    PGP_MANUAL_ZONEN( __ExportVerts, "ExportVerts" );
#if PACKED_VERTS
    // Quantization strategy described here: https://gpuopen.com/learn/mesh_shaders/mesh_shaders-meshlet_compression/
    const u32 TARGET_BITS               = 16;
    const vec3 globalMin                = modelAABB.min;
    const vec3 globalDelta              = modelAABB.max - modelAABB.min;
    const vec3 meshletQuantizationStep  = largestMeshletExtents / float( ( 1 << TARGET_BITS ) - 1 );
    const vec3 globalQuantizationStates = vec3( uvec3( globalDelta / meshletQuantizationStep ) );
    const vec3 effectiveBits            = Log2( globalQuantizationStates );
    const vec3 quantizationFactor       = ( globalQuantizationStates - vec3( 1 ) ) / globalDelta;
    LOG( "Effective Quantization Bits: %f %f %f", effectiveBits.x, effectiveBits.y, effectiveBits.z );

    positionDequantizationInfo.factor    = globalDelta / ( globalQuantizationStates - vec3( 1 ) );
    positionDequantizationInfo.globalMin = globalMin;
#endif // #if PACKED_VERTS

    for ( u32 meshIdx = 0; meshIdx < numMeshes; ++meshIdx )
    {
        Mesh& m                        = meshes[meshIdx];
        const PModel::Mesh& pMesh      = pmodel.meshes[meshIdx];
        const MeshBuildData& buildData = meshBuildDatas[meshIdx];

        std::atomic<u32> numZeroNormals  = 0;
        std::atomic<u32> numZeroTangents = 0;
        m.packedPositions.resize( buildData.numMeshletVerts );
#if PACKED_VERTS
        std::vector<u32> octNormals( buildData.numMeshletVerts );
        std::vector<u32> octTangents;
        if ( pMesh.hasTangents )
            octTangents.resize( buildData.numMeshletVerts );
#else  // #if PACKED_VERTS
        m.packedNormals.resize( buildData.numMeshletVerts );
        if ( pMesh.hasTangents )
            m.packedTangents.resize( buildData.numMeshletVerts );
#endif // #else // #if PACKED_VERTS
        if ( pMesh.numUVChannels > 0 )
            m.texCoords.resize( buildData.numMeshletVerts );

        int numBlocks = ROUND_UP_DIV( buildData.numMeshlets, 64 );
#pragma omp parallel for
        for ( int blockIdx = 0; blockIdx < numBlocks; ++blockIdx )
        {
            u32 meshletOffset = 64 * (u32)blockIdx;
            for ( u32 localMeshletIdx = 0; localMeshletIdx < 64; ++localMeshletIdx )
            {
                u32 meshletIdx = meshletOffset + localMeshletIdx;
                if ( meshletIdx >= buildData.numMeshlets )
                    break;
                GpuData::Meshlet& pgMeshlet = m.meshlets[meshletIdx];
#if PACKED_VERTS
                const uvec3 quantizedMeshletOffset = ( buildData.meshletAABBs[meshletIdx].min - globalMin ) * quantizationFactor + 0.5f;
                pgMeshlet.quantizedMeshletOffset   = quantizedMeshletOffset;
#endif // #if PACKED_VERTS

                for ( u32 localVIdx = 0; localVIdx < pgMeshlet.vertexCount; ++localVIdx )
                {
                    size_t globalVIdx       = pgMeshlet.vertexOffset + localVIdx;
                    const PModel::Vertex& v = pMesh.vertices[buildData.meshletVertices[globalVIdx]];

#if PACKED_VERTS
                    uvec3 globalQuantizedValue    = ( v.pos - globalMin ) * quantizationFactor + 0.5f;
                    uvec3 localQuantizedValue     = globalQuantizedValue - quantizedMeshletOffset;
                    m.packedPositions[globalVIdx] = u16vec3( localQuantizedValue );

                    u32 packedNormal       = OctEncodeSNorm_P( v.normal, BITS_PER_NORMAL / 2 );
                    octNormals[globalVIdx] = packedNormal;
#else  // #if PACKED_VERTS
                    m.packedPositions[globalVIdx] = v.pos;
                    m.packedNormals[globalVIdx]   = v.normal;
#endif // #else // #if PACKED_VERTS

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
                        vec3 tangent   = v.tangent;
                        vec3 bitangent = v.bitangent;
                        vec3 tNormal   = Cross( tangent, bitangent );
                        bool bSignPos  = Dot( v.normal, tNormal ) > 0.0f;
#if PACKED_VERTS
                        u32 packedTangent = OctEncodeSNorm_P( tangent, BITS_PER_TANGENT_COMPONENT );
                        packedTangent |= ( bSignPos ? 1u : 0u ) << ( BITS_PER_TANGENT - 1 );
                        octTangents[globalVIdx] = packedTangent;
#else  // #if PACKED_VERTS
                        m.packedTangents[globalVIdx] = vec4( tangent, bSignPos ? 1.0f : -1.0f );
#endif // #else // #if PACKED_VERTS

                        if ( Length( v.tangent ) <= 0.00001f )
                            ++numZeroTangents;
                    }
                }
            }
        }

#if PACKED_VERTS
        m.packedNormals.resize( ROUND_UP_DIV( BITS_PER_NORMAL * buildData.numMeshletVerts, 32 ), 0 );
        if ( pMesh.hasTangents )
            m.packedTangents.resize( ROUND_UP_DIV( BITS_PER_TANGENT * buildData.numMeshletVerts, 32 ), 0 );
        u64 normalBitOffset  = 0;
        u64 tangentBitOffset = 0;
        for ( u32 vIdx = 0; vIdx < buildData.numMeshletVerts; ++vIdx )
        {
            {
                u32 qn        = octNormals[vIdx];
                u64 pIdx      = normalBitOffset / 32;
                u32 shift     = normalBitOffset % 32;
                u32 remaining = 32 - shift;

                m.packedNormals[pIdx] |= qn << shift;
                if ( BITS_PER_NORMAL > remaining )
                    m.packedNormals[pIdx + 1] |= qn >> remaining;

                normalBitOffset += BITS_PER_NORMAL;
            }
            if ( pMesh.hasTangents )
            {
                u32 packedTB  = octTangents[vIdx];
                u64 pIdx      = tangentBitOffset / 32;
                u32 shift     = tangentBitOffset % 32;
                u32 remaining = 32 - shift;

                m.packedTangents[pIdx] |= packedTB << shift;
                if ( BITS_PER_TANGENT > remaining )
                    m.packedTangents[pIdx + 1] |= packedTB >> remaining;

                tangentBitOffset += BITS_PER_TANGENT;
            }
        }
#endif // #if PACKED_VERTS
        if ( numZeroNormals.load() )
            LOG_WARN( "Mesh[%u] '%s' inside of model '%s' has %u normals of length 0", meshIdx, pMesh.name.c_str(),
                createInfo->name.c_str(), numZeroNormals.load() );
        if ( numZeroTangents.load() )
            LOG_WARN( "Mesh[%u] '%s' inside of model '%s' has %u tangents of length 0", meshIdx, pMesh.name.c_str(),
                createInfo->name.c_str(), numZeroTangents.load() );
    }
    PGP_MANUAL_ZONE_END( __ExportVerts );

    if ( nonCompactibleMeshlets )
    {
        LOG_WARN( "%u/%u meshlets were incompactible for model '%s' and had to be split. Final meshlet count: %u", nonCompactibleMeshlets,
            totalMeshletsPreCompaction, createInfo->name.c_str(), totalMeshletsPostCompaction );
    }

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
        serializer->Read( mesh.packedNormals );
        serializer->Read( mesh.texCoords );
        serializer->Read( mesh.packedTangents );
        serializer->Read( mesh.packedTris );
        serializer->Read( mesh.meshlets );
        serializer->Read( mesh.meshletCullDatas );
#else // #if !USING( GPU_DATA )
        u64 numVerts        = serializer->Read<u64>();
        const void* posData = serializer->GetData();
        u64 posDataSize     = numVerts * ( PACKED_VERTS ? sizeof( u16vec3 ) : sizeof( vec3 ) );
        serializer->Skip( posDataSize );

        u64 numNormalChunks    = serializer->Read<u64>();
        const void* normalData = serializer->GetData();
        u64 normalDataSize     = PACKED_VERTS ? ( numNormalChunks * sizeof( u32 ) ) : ( numVerts * sizeof( vec3 ) );
        serializer->Skip( normalDataSize );

        u64 numTexCoords   = serializer->Read<u64>();
        const void* uvData = serializer->GetData();
        u64 uvDataSize     = numTexCoords * sizeof( vec2 );
        serializer->Skip( uvDataSize );

        u64 numTanChunks     = serializer->Read<u64>();
        const void* tanData  = serializer->GetData();
        u64 tangentsDataSize = PACKED_VERTS ? ( numTanChunks * sizeof( u32 ) ) : ( numVerts * sizeof( vec4 ) );
        serializer->Skip( tangentsDataSize );

        u64 numTrisOrIndices = serializer->Read<u64>();
        const void* triData  = serializer->GetData();
        u64 triDataSize      = numTrisOrIndices * ( PACKED_TRIS ? sizeof( u16 ) : sizeof( u8 ) );
        serializer->Skip( triDataSize );

        u64 numMeshlets         = serializer->Read<u64>();
        const void* meshletData = serializer->GetData();
        serializer->Skip( numMeshlets * sizeof( GpuData::Meshlet ) );

        const void* meshletCullData = serializer->GetData();
        serializer->Skip( numMeshlets * sizeof( GpuData::PackedMeshletCullData ) );

        mesh.numVertices  = static_cast<u32>( numVerts );
        mesh.numMeshlets  = static_cast<u32>( numMeshlets );
        mesh.hasTexCoords = numTexCoords != 0;
        mesh.hasTangents  = numTanChunks != 0;

        u64 totalVertexSizeInBytes = 0;
        totalVertexSizeInBytes += ROUND_UP_TO_MULT( posDataSize, 4 );
        totalVertexSizeInBytes += normalDataSize;
        totalVertexSizeInBytes += uvDataSize;
        totalVertexSizeInBytes += tangentsDataSize;

        BufferCreateInfo vbCreateInfo{};
        vbCreateInfo.size               = totalVertexSizeInBytes;
        vbCreateInfo.bufferUsage        = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        vbCreateInfo.addToBindlessArray = false; // since BindlessManager::AddMeshBuffers will take care of it

        BufferCreateInfo tbCreateInfo = vbCreateInfo;
        tbCreateInfo.size             = triDataSize;

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

        u64 offset = 0;
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], posData, posDataSize, offset );
        offset += ROUND_UP_TO_MULT( posDataSize, 4 );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], normalData, normalDataSize, offset );
        offset += normalDataSize;
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], uvData, uvDataSize, offset );
        offset += uvDataSize;
        rg.device.AddUploadRequest( mesh.buffers[Mesh::VERTEX_BUFFER], tanData, tangentsDataSize, offset );
        offset += tangentsDataSize;

        rg.device.AddUploadRequest( mesh.buffers[Mesh::TRI_BUFFER], triData, tbCreateInfo.size );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::MESHLET_BUFFER], meshletData, mbCreateInfo.size );
        rg.device.AddUploadRequest( mesh.buffers[Mesh::CULL_DATA_BUFFER], meshletCullData, cbCreateInfo.size );
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
        serializer->Write( mesh.packedNormals );
        serializer->Write( mesh.texCoords );
        serializer->Write( mesh.packedTangents );
        serializer->Write( mesh.packedTris );
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
        mesh.packedNormals    = {};
        mesh.texCoords        = {};
        mesh.packedTangents   = {};
        mesh.packedTris       = {};
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
