#include "r_lighting.hpp"
#include "c_shared/cull_data.h"
#include "c_shared/lights.h"
#include "c_shared/scene_globals.h"
#include "core/scene.hpp"
#include "r_dvars.hpp"
#include "r_pipeline_manager.hpp"
#include "shared/float_conversions.hpp"

namespace PG::Gfx::Lighting
{

struct ShadowedFrustum
{
    float dist;
    u16 lightIdx;
    LightType type;
};

struct LocalFrameData
{
    Gfx::Buffer packedLightBuffer;
    Gfx::Buffer lightFrustumsBuffer;
    u32 numLights;
    std::vector<ShadowedFrustum> shadowedLights;

    u32 numFrustumUpdates;
};

#define MAX_LIGHTS 65536
#define MAX_SHADOW_UPDATES_PER_FRAME 32
#define MAX_CULLED_OBJECTS_PER_FRUSTUM 65536

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];

inline LocalFrameData& LocalData() { return s_localFrameDatas[Gfx::rg.currentFrameIdx]; }

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].numLights = 0;

        BufferCreateInfo packedLightBuffCI     = {};
        packedLightBuffCI.size                 = MAX_LIGHTS * sizeof( GpuData::PackedLight );
        packedLightBuffCI.bufferUsage          = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        packedLightBuffCI.flags                = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        s_localFrameDatas[i].packedLightBuffer = rg.device.NewBuffer( packedLightBuffCI, "packedLightBuffer_" + std::to_string( i ) );

        BufferCreateInfo lightFrustumsBufferCI = {};
        lightFrustumsBufferCI.size             = MAX_SHADOW_UPDATES_PER_FRAME * 6 * sizeof( vec4 );
        lightFrustumsBufferCI.bufferUsage      = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        lightFrustumsBufferCI.flags            = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        s_localFrameDatas[i].lightFrustumsBuffer = rg.device.NewBuffer( packedLightBuffCI, "lightFrustumsBuffer_" + std::to_string( i ) );

        s_localFrameDatas[i].shadowedLights.reserve( 128 );
    }
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].packedLightBuffer.Free();
        s_localFrameDatas[i].lightFrustumsBuffer.Free();
    }
}

static f32 PackRadius( f32 radius )
{
    f32 inv    = 1.0f / radius;
    u32 packed = Float32ToFloat16( radius, inv * inv );
    return *reinterpret_cast<f32*>( &packed );
}

// NOTE! Keep the following Pack* functions in sync with lights.glsl
static GpuData::PackedLight PackLight( Light* light )
{
    GpuData::PackedLight p;
    p.colorAndType = vec4( light->intensity * light->color, Underlying( light->type ) );
    if ( light->type == LightType::POINT )
    {
        PointLight* l       = static_cast<PointLight*>( light );
        p.positionAndRadius = vec4( l->position, PackRadius( l->radius ) );
    }
    else if ( light->type == LightType::SPOT )
    {
        SpotLight* l             = static_cast<SpotLight*>( light );
        p.positionAndRadius      = vec4( l->position, PackRadius( l->radius ) );
        f32 cosOuterAngle        = cos( l->outerAngle );
        f32 invCosAngleDiff      = 1.0f / ( cos( l->innerAngle ) - cosOuterAngle );
        u32 packedAngles         = Float32ToFloat16( cosOuterAngle, invCosAngleDiff );
        p.directionAndSpotAngles = vec4( l->direction, *reinterpret_cast<f32*>( &packedAngles ) );
    }
    else
    {
        PG_ASSERT( false, "Invalid light type %u in PackLight", Underlying( light->type ) );
    }

    p.shadowMapIdx.x = light->castsShadow ? light->shadowMapTex->GetBindlessIndex() : 0;

    return p;
}

void UpdateLights( Scene* scene )
{
    GpuData::PackedLight* gpuLights = reinterpret_cast<GpuData::PackedLight*>( LocalData().packedLightBuffer.GetMappedPtr() );
    u32& numLights                  = LocalData().numLights;
    numLights                       = 0;
    auto& shadowedLights            = LocalData().shadowedLights;
    shadowedLights.clear();

    if ( scene->directionalLight )
    {
        GpuData::PackedLight p;
        p.colorAndType           = vec4( scene->directionalLight->intensity * scene->directionalLight->color, LIGHT_TYPE_DIRECTIONAL );
        p.directionAndSpotAngles = vec4( scene->directionalLight->direction, 0 );
        p.shadowMapIdx.x         = 0;
        gpuLights[numLights++]   = p;
    }

    u32 lightsToAdd = Min<u32>( MAX_LIGHTS - numLights, (u32)scene->lights.size() );
    for ( u32 i = 0; i < lightsToAdd; ++i )
    {
        Light* light           = scene->lights[i];
        gpuLights[numLights++] = PackLight( light );

        if ( light->castsShadow )
        {
            ShadowedFrustum& sf = shadowedLights.emplace_back();
            sf.lightIdx         = (u16)i;
            sf.type             = light->type;
            if ( light->type == LightType::POINT )
            {
                PointLight* l = static_cast<PointLight*>( light );
                sf.dist       = Length( l->position - scene->camera.position );
            }
            else if ( light->type == LightType::SPOT )
            {
                SpotLight* l = static_cast<SpotLight*>( light );
                sf.dist      = Length( l->position - scene->camera.position );
            }
        }
    }

    // std::sort( shadowedLights.begin(), shadowedLights.end(),
    //     []( const ShadowedFrustum& a, const ShadowedFrustum& b ) { return a.dist < b.dist; } );

    u32 numFrustumUpdates = 0;
    u32 numShadowedLights = 0;
    u16 shadowIndices[MAX_SHADOW_UPDATES_PER_FRAME];
    for ( u64 i = 0; i < shadowedLights.size() && numFrustumUpdates < MAX_SHADOW_UPDATES_PER_FRAME; ++i )
    {
        const ShadowedFrustum& sf = shadowedLights[i];
        u16 numFrustums           = sf.type == LightType::POINT ? 6 : 1;
        if ( numFrustumUpdates + numFrustums <= MAX_SHADOW_UPDATES_PER_FRAME )
        {
            shadowIndices[numShadowedLights] = (u16)i;
            numFrustumUpdates += numFrustums;
            ++numShadowedLights;
        }
    }

    if ( numShadowedLights != shadowedLights.size() )
    {
        LOG_WARN( "Maxinum of shadows updated per frame reached, some lights will not be updated" );
        for ( u32 i = 0; i < numShadowedLights; ++i )
        {
            shadowedLights[i] = shadowedLights[shadowIndices[i]];
        }

        shadowedLights.resize( numShadowedLights );
    }

    LocalData().numFrustumUpdates = numFrustumUpdates;
}

static const vec3 s_pointLightForwardDirs[6] = {
    vec3( 1, 0, 0 ),  // right
    vec3( -1, 0, 0 ), // left
    vec3( 0, 1, 0 ),  // forward
    vec3( 0, -1, 0 ), // back
    vec3( 0, 0, 1 ),  // top
    vec3( 0, 0, -1 ), // bottom
};

static const vec3 s_pointLightUpDirs[6] = {
    vec3( 0, 0, 1 ),  // right
    vec3( 0, 0, 1 ),  // left
    vec3( 0, 0, 1 ),  // forward
    vec3( 0, 0, 1 ),  // back
    vec3( 0, -1, 0 ), // top
    vec3( 0, 1, 0 ),  // bottom
};

#if USING( DEVELOPMENT_BUILD )
void ComputeShadowFrustumsCull_Debug( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "shadow_frustums_cull_debug" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress countBuffer;
        u32 numFrustums;
        u32 _pad;
    };
    ComputePushConstants push;
    push.countBuffer = task->GetInputBuffer( 0 ).GetDeviceAddress();
    push.numFrustums = LocalData().numFrustumUpdates;
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch( 1, 1, 1 );
}
#endif // #if USING( DEVELOPMENT_BUILD )

void ComputeShadowFrustumsCull( ComputeTask* task, TGExecuteData* data )
{
    CommandBuffer& cmdBuf = *data->cmdBuf;

    vec4 frustums[MAX_SHADOW_UPDATES_PER_FRAME][6];
    u32 numFrustums = 0;
    for ( u64 sIdx = 0; sIdx < LocalData().shadowedLights.size(); ++sIdx )
    {
        Light* light = data->scene->lights[LocalData().shadowedLights[sIdx].lightIdx];

        u32 frustumsPerLight = 1;
        if ( light->type == LightType::POINT )
        {
            frustumsPerLight = 6;
            PointLight* l    = static_cast<PointLight*>( light );
            mat4 P           = glm::perspective( PI / 2.0f, 1.0f, l->shadowNearPlane, l->radius );
            for ( u32 i = 0; i < 6; ++i )
            {
                mat4 V  = glm::lookAt( l->position, l->position + s_pointLightForwardDirs[i], s_pointLightUpDirs[i] );
                mat4 VP = P * V;
                ExtractPlanesFromVP( VP, frustums[numFrustums + i] );
            }
        }
        else if ( light->type == LightType::SPOT )
        {
            SpotLight* l = static_cast<SpotLight*>( light );
            mat4 P       = glm::perspective( 2.0f * l->outerAngle, 1.0f, l->shadowNearPlane, l->radius );
            vec3 up      = l->direction.z > 0.99 ? vec3( 0, -1, 0 ) : vec3( 0, 0, 1 );
            if ( l->direction.z < -0.99 )
                up = vec3( 0, 1, 0 );
            mat4 V  = glm::lookAt( l->position, l->position + l->direction, up );
            mat4 VP = P * V;
            ExtractPlanesFromVP( VP, frustums[numFrustums] );
        }

        numFrustums += frustumsPerLight;
    }
    PG_ASSERT( numFrustums <= MAX_SHADOW_UPDATES_PER_FRAME );

    vec4* gpuPlanes = LocalData().lightFrustumsBuffer.GetMappedPtr<vec4>();
    memcpy( gpuPlanes, frustums, numFrustums * 6 * sizeof( vec4 ) );

    GpuData::PerObjectData* meshDrawData = data->frameData->meshDrawDataBuffer.GetMappedPtr<GpuData::PerObjectData>();
    GpuData::CullData* cullData          = data->frameData->meshCullData.GetMappedPtr<GpuData::CullData>();
    cmdBuf.BindPipeline( PipelineManager::GetPipeline( "shadow_frustums_cull" ) );
    cmdBuf.BindGlobalDescriptors();

    struct ComputePushConstants
    {
        VkDeviceAddress cullDataBuffer;
        VkDeviceAddress frustumPlanesBuffer;
        VkDeviceAddress outputCountsBuffer;
        VkDeviceAddress indirectCmdOutputBuffer;
        u32 numMeshes;
        u32 numFrustums;
    };
    ComputePushConstants push;
    push.cullDataBuffer          = data->frameData->meshCullData.GetDeviceAddress();
    push.frustumPlanesBuffer     = LocalData().lightFrustumsBuffer.GetDeviceAddress();
    push.outputCountsBuffer      = task->GetOutputBuffer( 0 ).GetDeviceAddress();
    push.indirectCmdOutputBuffer = task->GetOutputBuffer( 1 ).GetDeviceAddress();
    push.numMeshes               = data->frameData->numMeshes;
    push.numFrustums             = numFrustums;
    cmdBuf.PushConstants( push );
    cmdBuf.Dispatch_AutoSized( push.numMeshes, 1, 1 );
}

void AddShadowTasks( TaskGraphBuilder& builder )
{
    ComputeTaskBuilder* cTask      = builder.AddComputeTask( "shadow_frustum_culling" );
    TGBBufferRef indirectCountBuff = cTask->AddBufferOutput(
        "indirectCountsBuff", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, MAX_SHADOW_UPDATES_PER_FRAME * sizeof( u32 ), 0 );
    TGBBufferRef indirectMeshletDrawBuff = cTask->AddBufferOutput( "indirectMeshletDrawBuff", VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        MAX_SHADOW_UPDATES_PER_FRAME * MAX_CULLED_OBJECTS_PER_FRUSTUM * sizeof( GpuData::MeshletDrawCommand ) );
    cTask->SetFunction( ComputeShadowFrustumsCull );

#if USING( DEVELOPMENT_BUILD )
    cTask = builder.AddComputeTask( "shadow_frustum_culling_debug" );
    cTask->AddBufferInput( indirectCountBuff );
    cTask->SetFunction( ComputeShadowFrustumsCull_Debug );
#endif // #if USING( DEVELOPMENT_BUILD )
}

u32 GetLightCount() { return LocalData().numLights; }

u64 GetLightBufferAddress() { return LocalData().packedLightBuffer.GetDeviceAddress(); }

} // namespace PG::Gfx::Lighting
