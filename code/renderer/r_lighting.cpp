#include "r_lighting.hpp"
#include "c_shared/lights.h"
#include "core/scene.hpp"
#include "r_dvars.hpp"
#include "r_pipeline_manager.hpp"
#include "shared/float_conversions.hpp"

namespace PG::Gfx::Lighting
{

struct LocalFrameData
{
    Gfx::Buffer packedLightBuffer;
    u32 numLights;
};

#define MAX_LIGHTS 65536

static LocalFrameData s_localFrameDatas[NUM_FRAME_OVERLAP];

inline LocalFrameData& LocalData() { return s_localFrameDatas[Gfx::rg.currentFrameIdx]; }

void Init()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        BufferCreateInfo packedLightBuffCI     = {};
        packedLightBuffCI.size                 = MAX_LIGHTS * sizeof( GpuData::PackedLight );
        packedLightBuffCI.bufferUsage          = BufferUsage::TRANSFER_DST | BufferUsage::STORAGE | BufferUsage::DEVICE_ADDRESS;
        packedLightBuffCI.flags                = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        s_localFrameDatas[i].packedLightBuffer = rg.device.NewBuffer( packedLightBuffCI, "packedLightBuffer_" + std::to_string( i ) );
        s_localFrameDatas[i].numLights         = 0;
    }
}

void Shutdown()
{
    for ( int i = 0; i < NUM_FRAME_OVERLAP; ++i )
    {
        s_localFrameDatas[i].packedLightBuffer.Free();
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
    if ( light->type == Light::Type::SPOT )
    {
        PointLight* l       = static_cast<PointLight*>( light );
        p.positionAndRadius = vec4( l->position, PackRadius( l->radius ) );
    }
    else if ( light->type == Light::Type::SPOT )
    {
        SpotLight* l             = static_cast<SpotLight*>( light );
        p.positionAndRadius      = vec4( l->position, PackRadius( l->radius ) );
        f32 cosOuterAngle        = cos( l->outerAngle );
        f32 invCosAngleDiff      = 1.0f / ( cos( l->innerAngle ) - cosOuterAngle );
        u32 packedAngles         = Float32ToFloat16( cosOuterAngle, invCosAngleDiff );
        p.directionAndSpotAngles = vec4( l->direction, *reinterpret_cast<f32*>( &packedAngles ) );
    }

    return p;
}

void UpdateLightBuffer( Scene* scene )
{
    GpuData::PackedLight* gpuLights = reinterpret_cast<GpuData::PackedLight*>( LocalData().packedLightBuffer.GetMappedPtr() );
    u32& numLights                  = LocalData().numLights;
    numLights                       = 0;

    if ( scene->directionalLight )
    {
        GpuData::PackedLight p;
        p.colorAndType           = vec4( scene->directionalLight->intensity * scene->directionalLight->color, LIGHT_TYPE_DIRECTIONAL );
        p.directionAndSpotAngles = vec4( scene->directionalLight->direction, 0 );
        gpuLights[numLights++]   = p;
    }

    u32 lightsToAdd = Min<u32>( MAX_LIGHTS - numLights, (u32)scene->lights.size() );
    for ( u32 i = 0; i < lightsToAdd; ++i )
    {
        gpuLights[numLights++] = PackLight( scene->lights[i] );
    }
}

u32 GetLightCount() { return LocalData().numLights; }

u64 GetLightBufferAddress() { return LocalData().packedLightBuffer.GetDeviceAddress(); }

} // namespace PG::Gfx::Lighting
