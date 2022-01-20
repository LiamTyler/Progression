#pragma once

#include "asset/types/model.hpp"
#include "asset/pt_material.hpp"
#include "core/bounding_box.hpp"
#include "ecs/components/transform.hpp"
#include "intersection_tests.hpp"
#include "pt_lights.hpp"
#include "shapes.hpp"
#include <memory>
#include <vector>

namespace PT
{

    extern std::vector< MeshInstance > g_meshInstances;

    class MeshInstance
    {
    public:
        MeshInstance( PG::Model* model, uint32_t meshIdx, const PG::Transform& localToWorld, MaterialHandle material );

        PG::Transform localToWorld;
        PG::AABB worldSpaceAABB;
        
        std::string name;
        MaterialHandle material;
        std::vector< glm::vec3 > positions;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
    };

    void AddMeshInstancesForModel( PG::Model* model, std::vector<PG::Material*> materials, const PG::Transform& transform );
    void EmitTrianglesForAllMeshes( std::vector< Shape* >& shapes, std::vector< Light* >& lights );

} // namespace PT