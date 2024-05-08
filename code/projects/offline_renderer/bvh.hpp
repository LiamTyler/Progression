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
        int firstIndexOffset;  // if node is a leaf
        int secondChildOffset; // if node is not a leaf (only saving the offset of the 2nd child, since the first child offset is always the
                               // parentIndex + 1)
    };
    uint16_t numShapes;
    uint8_t axis;
    uint8_t padding;
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
    bool Occluded( const Ray& ray, float tMax = FLT_MAX ) const;
    PG::AABB GetAABB() const;

    std::vector<Shape*> shapes;
    LinearBVHNode* nodes = nullptr;
};

} // namespace PT
