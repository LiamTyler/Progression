#pragma once

#include "asset/types/base_asset.hpp"
#include "glm/vec3.hpp"

class Serializer;

namespace PG
{

struct MaterialCreateInfo : public BaseAssetCreateInfo
{
    std::string texturesetName;
    glm::vec3 albedoTint;
    float metalnessTint;
    //float roughnessTint;
    glm::vec3 emissiveTint;
    
    bool applyAlbedo = true;
    bool applyMetalness = true;
    //bool applyNormals = true;
    //bool applyRoughness = true;
    bool applyEmissive = true;
};

struct GfxImage;

struct Material : public BaseAsset
{
    Material() = default;

    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    glm::vec3 albedoTint = glm::vec3( 1.0f );
    float metalnessTint  = 1.0f;
    GfxImage* albedoMetalnessImage = nullptr;

    //float roughnessTint;
    //GfxImage* normalRoughnessImage = nullptr;

    glm::vec3 emissiveTint = glm::vec3( 0.0f );
    GfxImage* emissiveImage = nullptr;
};

} // namespace PG
