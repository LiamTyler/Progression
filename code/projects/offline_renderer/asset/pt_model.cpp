#include "asset/pt_model.hpp"
#include "shapes.hpp"
#include "shared/logger.hpp"

using namespace PG;

namespace PT
{

std::vector<MeshInstance> g_meshInstances = { MeshInstance() };

MeshInstance::MeshInstance() : material( MATERIAL_HANDLE_INVALID ) {}

MeshInstance::MeshInstance( Model* model, u32 meshIdx, const Transform& inLocalToWorld, MaterialHandle inMaterial )
{
    PG_ASSERT( false, "todo: update to handle new meshlet models" );
    /*
    localToWorld         = inLocalToWorld;
    u32 numVertices = model->meshes[meshIdx].numVertices;
    positions.resize( numVertices );
    normals.resize( numVertices );
    tangents.resize( numVertices );
    for ( u32 i = 0; i < numVertices; ++i )
    {
        u32 startVert = model->meshes[meshIdx].startVertex;
        positions[i]       = localToWorld.TransformPoint( model->positions[startVert + i] );
        mat4 N             = Inverse( Transpose( localToWorld.Matrix() ) );
        normals[i]         = Normalize( vec3( N * vec4( model->normals[startVert + i], 0 ) ) );
        tangents[i]        = Normalize( localToWorld.TransformVector( model->tangents[startVert + i] ) );
    }
    if ( model->texCoords.size() )
    {
        uvs.resize( numVertices );
        for ( u32 i = 0; i < numVertices; ++i )
        {
            u32 startVert = model->meshes[meshIdx].startVertex;
            uvs[i]             = model->texCoords[startVert + i];
        }
    }
    indices.resize( model->meshes[meshIdx].numIndices );
    for ( u32 i = 0; i < model->meshes[meshIdx].numIndices; ++i )
    {
        u32 startIndex = model->meshes[meshIdx].startIndex;
        indices[i]          = model->indices[startIndex + i];
    }
    material = inMaterial;
    */
}

void AddMeshInstancesForModel( Model* model, std::vector<PG::Material*> materials, const Transform& transform )
{
    PG_ASSERT( false, "todo: update to handle new meshlet models" );
    /*
    if ( model->texCoords.empty() )
    {
        model->texCoords.resize( model->positions.size(), vec2( 0 ) );
    }
    else if ( model->tangents.empty() )
    {
        LOG_WARN( "Model had texture coordinates, but no tangents. This should have generated tangents earlier, "
                  "in the converter. Using an arbitrary tangent space instead" );
    }

    if ( model->tangents.empty() )
    {
        model->tangents.resize( model->positions.size(), vec4( 0 ) );
        for ( const Mesh& mesh : model->meshes )
        {
            for ( u32 i = 0; i < mesh.numIndices; i += 3 )
            {
                const auto i0 = mesh.startVertex + model->indices[mesh.startIndex + i + 0];
                const auto i1 = mesh.startVertex + model->indices[mesh.startIndex + i + 1];
                const auto i2 = mesh.startVertex + model->indices[mesh.startIndex + i + 2];
                vec3 e        = model->positions[i1] - model->positions[i0];
                vec3 t        = Normalize( e );
                vec3 n        = model->normals[i0];

                if ( model->tangents[i0] == vec4( 0 ) )
                    model->tangents[i0] = vec4( Normalize( t - model->normals[i0] * Dot( model->normals[i0], t ) ), 1 );
                if ( model->tangents[i1] == vec4( 0 ) )
                    model->tangents[i1] = vec4( Normalize( t - model->normals[i1] * Dot( model->normals[i1], t ) ), 1 );
                if ( model->tangents[i2] == vec4( 0 ) )
                    model->tangents[i2] = vec4( Normalize( t - model->normals[i2] * Dot( model->normals[i2], t ) ), 1 );
            }
        }

        for ( size_t i = 0; i < model->tangents.size(); ++i )
        {
            vec3 t = model->tangents[i];
            if ( t == vec3( 0 ) || any( isinf( t ) ) || any( isnan( t ) ) )
            {
                LOG_WARN( "Tangent %zu of model %s is bad, setting manually", i, model->name.c_str() );
                model->tangents[i] = vec4( 1, 0, 0, 1 );
                if ( vec3( t ) == model->normals[i] )
                {
                    model->tangents[i] = vec4( 0, 1, 0, 1 );
                }
            }
        }
    }

    for ( u32 i = 0; i < static_cast<u32>( model->meshes.size() ); ++i )
    {
        g_meshInstances.emplace_back( model, i, transform, LoadMaterialFromPGMaterial( materials[i] ) );
    }
    */
}

void EmitTrianglesForAllMeshes( std::vector<Shape*>& shapes, std::vector<Light*>& lights )
{
    size_t newShapes = 0;
    size_t newLights = 0;
    for ( MeshInstanceHandle meshHandle = 1; meshHandle < static_cast<MeshInstanceHandle>( g_meshInstances.size() ); ++meshHandle )
    {
        const MeshInstance& mesh = g_meshInstances[meshHandle];
        const Material* material = GetMaterial( mesh.material );
        if ( material->isDecal )
            continue;

        newShapes += mesh.indices.size() / 3;
        if ( material && material->emissiveTint != vec3( 0 ) )
        {
            newLights += mesh.indices.size() / 3;
        }
    }
    shapes.reserve( shapes.size() + newShapes );
    lights.reserve( lights.size() + newLights );

    for ( MeshInstanceHandle meshHandle = 1; meshHandle < static_cast<MeshInstanceHandle>( g_meshInstances.size() ); ++meshHandle )
    {
        const MeshInstance& mesh = g_meshInstances[meshHandle];
        const Material* material = GetMaterial( mesh.material );
        if ( material->isDecal )
            continue;

        const bool isEmissive = material && material->emissiveTint != vec3( 0 );
        for ( u32 face = 0; face < static_cast<u32>( mesh.indices.size() / 3 ); ++face )
        {
            auto tri        = new Triangle;
            tri->meshHandle = meshHandle;
            tri->i0         = mesh.indices[3 * face + 0];
            tri->i1         = mesh.indices[3 * face + 1];
            tri->i2         = mesh.indices[3 * face + 2];
            shapes.push_back( tri );
            // if ( isEmissive )
            //{
            //     auto areaLight   = new AreaLight;
            //     areaLight->Lemit = material->emissiveTint;
            //     areaLight->shape = tri;
            //     lights.push_back( areaLight );
            // }
        }
    }
}

MeshInstance* GetMeshInstance( MeshInstanceHandle handle ) { return &g_meshInstances[handle]; }

} // namespace PT
