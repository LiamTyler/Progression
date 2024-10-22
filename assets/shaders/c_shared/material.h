#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "c_shared/defines.h"

BEGIN_GPU_DATA_NAMESPACE()

struct PackedMaterial
{
    vec3 albedoTint;
    float metalnessTint;
    
    float roughnessTint;
    uint albedoMetalnessMapIndex;
    uint normalRoughnessMapIndex;
    float _pad2;
    
    vec3 emissiveTint;
    uint emissiveMapIndex;
    
    vec4 MorePad;
};

END_GPU_DATA_NAMESPACE()

#endif // #ifndef __MATERIAL_H__