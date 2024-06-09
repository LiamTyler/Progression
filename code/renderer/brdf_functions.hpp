#pragma once

#include "shared/math_vec.hpp"

namespace PG
{

vec3 FresnelSchlick( f32 NdotV, vec3 F0 );

f32 GGX_D( f32 NdotH, f32 roughness );

// Samples just the distribution function D of GGX. Xi is a 2d random vec
vec3 ImportanceSampleGGX_D( vec2 Xi, vec3 N, f32 linearRoughness );

f32 GeometrySchlickGGX_IBL( f32 NdotV, f32 perceptualRoughness );
f32 GeometrySmith_IBL( vec3 N, vec3 V, vec3 L, f32 perceptualRoughness );

f32 GeometrySchlickGGX_Direct( f32 NdotV, f32 perceptualRoughness );
f32 GeometrySmith_Direct( vec3 N, vec3 V, vec3 L, f32 perceptualRoughness );

f32 V_SmithGGXCorrelated( f32 NoV, f32 NoL, f32 linearRoughness );

} // namespace PG
