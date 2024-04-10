#pragma once

#include "shared/math_vec.hpp"

namespace PG
{

vec3 FresnelSchlick( float NdotV, vec3 F0 );

float GGX_D( float NdotH, float roughness );

// Samples just the distribution function D of GGX. Xi is a 2d random vec
vec3 ImportanceSampleGGX_D( vec2 Xi, vec3 N, float linearRoughness );

float GeometrySchlickGGX_IBL( float NdotV, float perceptualRoughness );
float GeometrySmith_IBL( vec3 N, vec3 V, vec3 L, float perceptualRoughness );

float GeometrySchlickGGX_Direct( float NdotV, float perceptualRoughness );
float GeometrySmith_Direct( vec3 N, vec3 V, vec3 L, float perceptualRoughness );

float V_SmithGGXCorrelated( float NoV, float NoL, float linearRoughness );

} // namespace PG
