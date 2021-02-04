#pragma once

#include "asset/types/base_asset.hpp"
#include "glm/vec3.hpp"

class Serializer;

namespace PG
{

struct MaterialCreateInfo
{
    std::string name;
    glm::vec3 albedoTint;
    std::string albedoMapName;
    float metalnessTint;
    std::string metalnessMapName;
    float roughnessTint;
    std::string roughnessMapName;
};

struct GfxImage;

struct Material : public Asset
{
    Material() = default;
    glm::vec3 albedoTint = glm::vec3( 1.0f );
    float metalnessTint  = 1.0f;
    float roughnessTint  = 1.0f;

    GfxImage* albedoMap = nullptr;
    GfxImage* metalnessMap = nullptr;
    GfxImage* roughnessMap = nullptr;
};

bool Material_Load( Material* material, const MaterialCreateInfo& createInfo );

bool Fastfile_Material_Load( Material* material, Serializer* serializer );

bool Fastfile_Material_Save( const MaterialCreateInfo * const matcreateInfo, Serializer* serializer );

} // namespace PG
