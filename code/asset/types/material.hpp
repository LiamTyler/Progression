#pragma once

#include "asset/types/base_asset.hpp"
#include "glm/vec3.hpp"

class Serializer;

namespace PG
{

struct MaterialCreateInfo : public BaseAssetCreateInfo
{
    glm::vec3 albedoTint;
    std::string albedoMapName;
    float metalnessTint;
    std::string metalnessMapName;
    float roughnessTint;
    std::string roughnessMapName;
};

struct GfxImage;

struct Material : public BaseAsset
{
    Material() = default;
    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    glm::vec3 albedoTint = glm::vec3( 1.0f );
    float metalnessTint  = 1.0f;
    float roughnessTint  = 1.0f;

    GfxImage* albedoMap = nullptr;
    GfxImage* metalnessMap = nullptr;
    GfxImage* roughnessMap = nullptr;
};

} // namespace PG
