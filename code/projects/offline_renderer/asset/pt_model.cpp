#include "asset/pt_model.hpp"

using namespace PG;

namespace PT
{

    std::vector< MeshInstance > g_meshInstances;

    MeshInstance::MeshInstance( Model* model, uint32_t meshIdx, const Transform& inLocalToWorld, MaterialHandle inMaterial )   
    {
        localToWorld = inLocalToWorld;
        uint32_t numVertices = model->meshes[meshIdx].numVertices;
        positions.resize( numVertices );
        normals.resize( numVertices );
        tangents.resize( numVertices );
        for ( uint32_t i = 0; i < numVertices; ++i )
        {
            uint32_t startVert = model->meshes[meshIdx].startVertex;
            positions[i] = localToWorld.TransformPoint( model->positions[startVert +i] );
            glm::mat4 N = glm::inverse( glm::transpose( localToWorld.Matrix() ) );
            normals[i] = glm::normalize( glm::vec3( N * glm::vec4( model->normals[startVert +i], 0 ) ) );
            tangents[i] = glm::normalize( localToWorld.TransformVector( model->positions[startVert +i] ) );
        }
        if ( model->texCoords.size() )
        {
            uvs.resize( numVertices );
            for ( uint32_t i = 0; i < numVertices; ++i )
            {
                uint32_t startVert = model->meshes[meshIdx].startVertex;
                uvs[i] = model->texCoords[startVert +i];
            }
        }
        indices.resize( model->meshes[meshIdx].numIndices );
        for ( uint32_t i = 0; i < model->meshes[meshIdx].numIndices; ++i )
        {
            uint32_t startIndex = model->meshes[meshIdx].startIndex;
            indices[i] = model->indices[startIndex + i];
        }
        material = inMaterial;
    }


    void AddMeshInstancesForModel( Model* model, std::vector<PG::Material*> materials, const Transform& transform )
    {
        for ( uint32_t i = 0; i < static_cast<uint32_t>( model->meshes.size() ); ++i )
        {
            g_meshInstances.emplace_back( model, i, transform, LoadMaterialFromPGMaterial( materials[i] ) );
        }
    }


    void EmitTrianglesForAllMeshes( std::vector< Shape* >& shapes, std::vector< Light* >& lights )
    {
        for ( uint32_t meshIndex = 0; meshIndex < static_cast<uint32_t>( g_meshInstances.size() ); ++meshIndex )
        {
            const MeshInstance& mesh = g_meshInstances[meshIndex];
            shapes.reserve( shapes.size() + mesh.indices.size() / 3 );
            //if ( material->Ke != glm::vec3( 0 ) )
            //{
            //    lights.reserve( lights.size() + data.indices.size() / 3 );
            //}
        }

        for ( uint32_t meshIndex = 0; meshIndex < static_cast<uint32_t>( g_meshInstances.size() ); ++meshIndex )
        {
            const MeshInstance& mesh = g_meshInstances[meshIndex];
            for ( uint32_t face = 0; face < static_cast<uint32_t>( mesh.indices.size() / 3 ); ++face )
            {
                auto tri            = new Triangle;
                tri->meshIndex      = meshIndex;
                tri->firstVertIndex = 3 * face;
                shapes.push_back( tri );
                //if ( data.material->Ke != glm::vec3( 0 ) )
                //{
                //    auto areaLight   = new AreaLight;
                //    areaLight->Lemit = data.material->Ke;
                //    areaLight->shape = tri;
                //    lights.push_back( areaLight );
                //}
            }
        }
    }

} // namespace PT
