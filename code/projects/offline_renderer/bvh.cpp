#include "bvh.hpp"
#include "shared/assert.hpp"
#include <algorithm>

using PG::AABB;
using ShapePtr = PT::Shape*;

namespace PT
{

BVH::~BVH()
{
    if ( nodes )
    {
        delete[] nodes;
    }
}

struct BVHBuildNode
{
    std::unique_ptr<BVHBuildNode> firstChild;
    std::unique_ptr<BVHBuildNode> secondChild;
    u32 firstIndex;
    u32 numShapes;
    AABB aabb = AABB();
    u8 axis;
};

// struct to cache AABB info needed during bvh build
struct BVHBuildShapeInfo
{
    AABB aabb;
    vec3 centroid;
    ShapePtr shape;
};

static std::unique_ptr<BVHBuildNode> BuildBVHInteral( std::vector<BVHBuildShapeInfo>& buildShapeInfos, i32 start, i32 end,
    std::vector<ShapePtr>& orderedShapes, u32& totalNodes, BVH::SplitMethod splitMethod )
{
    auto node = std::make_unique<BVHBuildNode>();
    ++totalNodes;

    // calculate bounding box of all triangles
    for ( i32 i = start; i < end; ++i )
    {
        node->aabb.Encompass( buildShapeInfos[i].aabb );
    }

    i32 numShapes = end - start;
    PG_ASSERT( numShapes > 0 );
    if ( numShapes == 1 )
    {
        node->firstIndex = static_cast<u32>( orderedShapes.size() );
        node->numShapes  = end - start;
        for ( i32 i = start; i < end; ++i )
        {
            orderedShapes.push_back( buildShapeInfos[i].shape );
        }
        return node;
    }

    // split using the longest dimension of the aabb containing the centroids
    AABB centroidAABB;
    for ( i32 i = start; i < end; ++i )
    {
        centroidAABB.Encompass( buildShapeInfos[i].centroid );
    }
    i32 dim    = centroidAABB.LongestDimension();
    node->axis = dim;

    // sort all the triangles
    BVHBuildShapeInfo* beginShape = &buildShapeInfos[start];
    BVHBuildShapeInfo* endShape   = &buildShapeInfos[0] + end;
    BVHBuildShapeInfo* midShape;

    switch ( splitMethod )
    {
    case BVH::SplitMethod::Middle:
    {
        f32 mid = ( centroidAABB.min[dim] + centroidAABB.max[dim] ) / 2;
        midShape =
            std::partition( beginShape, endShape, [dim, mid]( const BVHBuildShapeInfo& shape ) { return shape.centroid[dim] < mid; } );

        // if the split did nothing, fall back to just splitting the nodes equally into two categories
        if ( midShape != endShape && midShape != beginShape )
        {
            break;
        }
    }
    case BVH::SplitMethod::EqualCounts:
    {
        midShape = &buildShapeInfos[( start + end ) / 2];
        std::nth_element( beginShape, midShape, endShape,
            [dim]( const BVHBuildShapeInfo& a, const BVHBuildShapeInfo& b ) { return a.centroid[dim] < b.centroid[dim]; } );
        break;
    }
    case BVH::SplitMethod::SAH:
    default:
    {
        // if there are only a few primitives, it doesnt really matter to bother with SAH
        if ( numShapes <= 4 )
        {
            midShape = &buildShapeInfos[( start + end ) / 2];
            std::nth_element( beginShape, midShape, endShape,
                [dim]( const BVHBuildShapeInfo& a, const BVHBuildShapeInfo& b ) { return a.centroid[dim] < b.centroid[dim]; } );
        }
        else
        {
            const i32 nBuckets = 12;
            struct BucketInfo
            {
                AABB aabb;
                i32 count = 0;
            };
            BucketInfo buckets[nBuckets];

            // bin all of the primitives into their corresponding buckets and update the bucket AABB
            for ( i32 i = start; i < end; ++i )
            {
                i32 b = std::min( nBuckets - 1, static_cast<i32>( nBuckets * centroidAABB.Offset( buildShapeInfos[i].centroid )[dim] ) );
                buckets[b].count++;
                buckets[b].aabb.Encompass( buildShapeInfos[i].aabb );
            }

            f32 cost[nBuckets - 1];
            // Compute costs for splitting after each bucket
            for ( i32 i = 0; i < nBuckets - 1; ++i )
            {
                AABB b0, b1;
                i32 count0 = 0, count1 = 0;
                for ( i32 j = 0; j <= i; ++j )
                {
                    b0.Encompass( buckets[j].aabb );
                    count0 += buckets[j].count;
                }
                for ( i32 j = i + 1; j < nBuckets; ++j )
                {
                    b1.Encompass( buckets[j].aabb );
                    count1 += buckets[j].count;
                }
                cost[i] = 0.5f + ( count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea() ) / node->aabb.SurfaceArea();
            }

            f32 minCost            = cost[0];
            i32 minCostSplitBucket = 0;
            for ( i32 i = 1; i < nBuckets - 1; ++i )
            {
                if ( cost[i] < minCost )
                {
                    minCost            = cost[i];
                    minCostSplitBucket = i;
                }
            }

            // see if the split is actually worth it
            f32 leafCost         = (f32)numShapes;
            i32 maxShapesPerLeaf = 4;
            if ( numShapes > maxShapesPerLeaf || minCost < leafCost )
            {
                midShape = std::partition( beginShape, endShape,
                    [&]( const BVHBuildShapeInfo& tri )
                    {
                        i32 b = std::min( nBuckets - 1, static_cast<i32>( nBuckets * centroidAABB.Offset( tri.centroid )[dim] ) );
                        return b <= minCostSplitBucket;
                    } );
            }
            else
            {
                node->firstIndex = static_cast<u32>( orderedShapes.size() );
                node->numShapes  = end - start;
                for ( i32 i = start; i < end; ++i )
                {
                    orderedShapes.push_back( buildShapeInfos[i].shape );
                }
                return node;
            }
        }
    }
    }

    i32 cutoff        = start + static_cast<i32>( midShape - &buildShapeInfos[start] );
    node->firstChild  = BuildBVHInteral( buildShapeInfos, start, cutoff, orderedShapes, totalNodes, splitMethod );
    node->secondChild = BuildBVHInteral( buildShapeInfos, cutoff, end, orderedShapes, totalNodes, splitMethod );

    return node;
}

static i32 FlattenBVHBuild( LinearBVHNode* linearRoot, BVHBuildNode* buildNode, u32& slot )
{
    if ( !buildNode )
    {
        return -1;
    }

    i32 currentSlot                   = slot++;
    linearRoot[currentSlot].aabb      = buildNode->aabb;
    linearRoot[currentSlot].axis      = buildNode->axis;
    linearRoot[currentSlot].numShapes = buildNode->numShapes;
    if ( buildNode->numShapes > 0 )
    {
        linearRoot[currentSlot].firstIndexOffset = buildNode->firstIndex;
    }
    else
    {
        FlattenBVHBuild( linearRoot, buildNode->firstChild.get(), slot );
        linearRoot[currentSlot].secondChildOffset = FlattenBVHBuild( linearRoot, buildNode->secondChild.get(), slot );
    }

    return currentSlot;
}

void BVH::Build( std::vector<ShapePtr>& listOfShapes, SplitMethod splitMethod )
{
    shapes = std::move( listOfShapes );
    if ( shapes.size() == 0 )
    {
        nodes            = new LinearBVHNode[1];
        nodes->aabb.min  = vec3( FLT_MAX );
        nodes->aabb.max  = vec3( FLT_MAX );
        nodes->numShapes = 0;
        return;
    }

    std::vector<BVHBuildShapeInfo> buildShapes( shapes.size() );
    for ( size_t i = 0; i < shapes.size(); ++i )
    {
        buildShapes[i].aabb     = shapes[i]->WorldSpaceAABB();
        buildShapes[i].centroid = buildShapes[i].aabb.Center();
        buildShapes[i].shape    = shapes[i];
    }

    std::vector<ShapePtr> orderedShapes;
    orderedShapes.reserve( shapes.size() );
    u32 totalNodes     = 0;
    auto buildRootNode = BuildBVHInteral( buildShapes, 0, static_cast<i32>( shapes.size() ), orderedShapes, totalNodes, splitMethod );
    shapes             = std::move( orderedShapes );

    // flatten the bvh
    nodes    = new LinearBVHNode[totalNodes];
    u32 slot = 0;
    FlattenBVHBuild( nodes, buildRootNode.get(), slot );
    PG_ASSERT( slot == totalNodes );
}

bool BVH::Intersect( const Ray& ray, IntersectionData* hitData ) const
{
    i32 nodesToVisit[64];
    i32 currentNodeIndex = 0;
    i32 toVisitOffset    = 0;
    vec3 invRayDir       = vec3( 1.0 ) / ray.direction;
    i32 isDirNeg[3]      = { invRayDir.x < 0, invRayDir.y < 0, invRayDir.z < 0 };
    f32 oldMaxT          = hitData->t;

    while ( true )
    {
        const LinearBVHNode& node = nodes[currentNodeIndex];
        if ( intersect::RayAABBFastest( ray.position, invRayDir, isDirNeg, node.aabb.min, node.aabb.max, hitData->t ) )
        {
            // if this  is a leaf node, check each triangle
            if ( node.numShapes > 0 )
            {
                for ( i32 shapeIndex = node.firstIndexOffset; shapeIndex < node.firstIndexOffset + node.numShapes; ++shapeIndex )
                {
                    shapes[shapeIndex]->Intersect( ray, hitData );
                }
                if ( toVisitOffset == 0 )
                    break;
                currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
            else
            {
                nodesToVisit[toVisitOffset++] = node.secondChildOffset;
                currentNodeIndex              = currentNodeIndex + 1;
            }
        }
        else
        {
            if ( toVisitOffset == 0 )
                break;
            currentNodeIndex = nodesToVisit[--toVisitOffset];
        }
    }

    return hitData->t < oldMaxT;
}

bool BVH::Occluded( const Ray& ray, f32 tMax ) const
{
    i32 nodesToVisit[64];
    i32 currentNodeIndex = 0;
    i32 toVisitOffset    = 0;
    vec3 invRayDir       = vec3( 1.0 ) / ray.direction;
    i32 isDirNeg[3]      = { invRayDir.x < 0, invRayDir.y < 0, invRayDir.z < 0 };

    while ( true )
    {
        const LinearBVHNode& node = nodes[currentNodeIndex];
        if ( intersect::RayAABBFastest( ray.position, invRayDir, isDirNeg, node.aabb.min, node.aabb.max, tMax ) )
        {
            // if this  is a leaf node, check each triangle
            if ( node.numShapes > 0 )
            {
                for ( i32 shapeIndex = node.firstIndexOffset; shapeIndex < node.firstIndexOffset + node.numShapes; ++shapeIndex )
                {
                    if ( shapes[shapeIndex]->TestIfHit( ray, tMax ) )
                    {
                        return true;
                    }
                }
                if ( toVisitOffset == 0 )
                    break;
                currentNodeIndex = nodesToVisit[--toVisitOffset];
            }
            else
            {
                nodesToVisit[toVisitOffset++] = node.secondChildOffset;
                currentNodeIndex              = currentNodeIndex + 1;
            }
        }
        else
        {
            if ( toVisitOffset == 0 )
                break;
            currentNodeIndex = nodesToVisit[--toVisitOffset];
        }
    }

    return false;
}

AABB BVH::GetAABB() const
{
    PG_ASSERT( nodes );
    return nodes[0].aabb;
}

} // namespace PT
