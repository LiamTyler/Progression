#pragma once

#include "glm/glm.hpp"

namespace PG
{

// Samples just the distribution function D of GGX. Xi is a 2d random vec
glm::vec3 ImportanceSampleGGX_D( glm::vec2 Xi, glm::vec3 N, float roughness );

float GeometrySchlickGGX_IBL( float NdotV, float roughness );
float GeometrySmith_IBL( glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness );

float GeometrySchlickGGX_Direct( float NdotV, float roughness );
float GeometrySmith_Direct( glm::vec3 N, glm::vec3 V, glm::vec3 L, float roughness );

} // namespace PG