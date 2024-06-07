#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "c_shared/defines.h"

#ifndef PG_SHADER_CODE
namespace GpuData
{
#endif // #ifndef PG_SHADER_CODE

struct Material
{
    VEC3 albedoTint;
    float metalnessTint;
    
    float roughnessTint;
    UINT albedoMetalnessMapIndex;
    UINT normalRoughnessMapIndex;
    float _pad2;
    
    VEC3 emissiveTint;
    UINT emissiveMapIndex;
    
    VEC4 MorePad;
};

#ifndef PG_SHADER_CODE
} // namespace GpuData
#endif // #ifndef PG_SHADER_CODE

#endif // #ifndef __MATERIAL_H__