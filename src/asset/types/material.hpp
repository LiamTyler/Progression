#pragma once

#include "asset/types/base_asset.hpp"
#include "glm/vec3.hpp"

class Serializer;

namespace PG
{

struct MaterialCreateInfo
{
    std::string name;
    glm::vec3 albedo;
    std::string albedoMapName;
    float metalness;
    std::string metalnessMapName;
    float roughness;
    std::string roughnessMapName;
};

struct GfxImage;

struct Material : public Asset
{
    Material() = default;
    glm::vec3 albedo = glm::vec3( 0 );
    float metalness  = 0;
    float roughness  = 1.0f;

    GfxImage* albedoMap = nullptr;
    GfxImage* metalnessMap = nullptr;
    GfxImage* roughnessMap = nullptr;
};

bool Material_Load( Material* material, const MaterialCreateInfo& createInfo );

bool Fastfile_Material_Load( Material* material, Serializer* serializer );

bool Fastfile_Material_Save( const MaterialCreateInfo * const matcreateInfo, Serializer* serializer );

} // namespace PG
