#pragma once

#include "asset/pt_material.hpp"
#include "asset/types/model.hpp"
#include "core/bounding_box.hpp"
#include "ecs/components/transform.hpp"
#include "intersection_tests.hpp"
#include "pt_lights.hpp"
#include <memory>
#include <vector>

namespace PT
{

class MeshInstance
{
public:
    MeshInstance();
    MeshInstance( PG::Model* model, u32 meshIdx, const PG::Transform& localToWorld, MaterialHandle material );

    PG::Transform localToWorld;
    PG::AABB worldSpaceAABB;

    std::string name;
    MaterialHandle material;
    std::vector<vec3> positions;
    std::vector<vec3> normals;
    std::vector<vec2> uvs;
    std::vector<vec3> tangents;
    std::vector<u32> indices;
};

struct Shape;
void AddMeshInstancesForModel( PG::Model* model, std::vector<PG::Material*> materials, const PG::Transform& transform );
void EmitTrianglesForAllMeshes( std::vector<Shape*>& shapes, std::vector<Light*>& lights );

using MeshInstanceHandle                                  = u32;
constexpr MeshInstanceHandle MESH_INSTANCE_HANDLE_INVALID = 0;

MeshInstance* GetMeshInstance( MeshInstanceHandle handle );

} // namespace PT
