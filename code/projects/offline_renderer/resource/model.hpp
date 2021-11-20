#pragma once

#include "aabb.hpp"
#include "intersection_tests.hpp"
#include "math.hpp"
#include "resource/material.hpp"
#include "resource/resource.hpp"
#include "shapes.hpp"
#include "transform.hpp"
#include <memory>
#include <vector>

namespace PT
{

    struct ModelCreateInfo
    {
        std::string name;
        std::string filename;
        bool recalculateNormals = true;
    };

    struct Mesh
    {
    public:
        std::string name;
        std::shared_ptr< Material > material;
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;

        void RecalculateNormals();
    };

    class Model : public Resource
    {
    public:
        Model() = default;
        
        bool Load( const ModelCreateInfo& createInfo );
        void RecalculateNormals();
        
        std::vector< Mesh > meshes;
    };

    class MeshInstance
    {
    public:
        MeshInstance( const Mesh& mesh, const Transform& localToWorld, std::shared_ptr< Material > material = nullptr );

        void EmitTrianglesAndLights( std::vector< std::shared_ptr< Shape > >& shapes, std::vector< Light* >& lights, std::shared_ptr< MeshInstance > meshPtr ) const;

        Transform localToWorld, worldToLocal;
        AABB worldSpaceAABB;
        Mesh data;
    };

} // namespace PT