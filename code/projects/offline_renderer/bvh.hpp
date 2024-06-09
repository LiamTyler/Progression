#pragma once

#include "core/bounding_box.hpp"
#include "shapes.hpp"
#include <vector>

namespace PT
{

struct LinearBVHNode
{
    PG::AABB aabb;
    union
    {
        i32 firstIndexOffset;  // if node is a leaf
        i32 secondChildOffset; // if node is not a leaf (only saving the offset of the 2nd child, since the first child offset is always the
                               // parentIndex + 1)
    };
    u16 numShapes;
    u8 axis;
    u8 padding;
};

class BVH
{
public:
    enum class SplitMethod
    {
        SAH,
        Middle,
        EqualCounts
    };

    BVH() = default;
    ~BVH();

    void Build( std::vector<Shape*>& shapes, SplitMethod splitMethod = SplitMethod::SAH );
    bool Intersect( const Ray& ray, IntersectionData* hitData ) const;
    bool Occluded( const Ray& ray, f32 tMax = FLT_MAX ) const;
    PG::AABB GetAABB() const;

    std::vector<Shape*> shapes;
    LinearBVHNode* nodes = nullptr;
};

} // namespace PT
