#pragma once

#include "asset/types/base_asset.hpp"
#include "glm/vec3.hpp"

class Serializer;

namespace PG
{

enum class MaterialType : uint8_t
{
    SURFACE = 0,
    DECAL = 1,

    COUNT
};


struct MaterialCreateInfo : public BaseAssetCreateInfo
{
    MaterialType type = MaterialType::SURFACE;
    std::string texturesetName;
    glm::vec3 albedoTint = glm::vec3( 1 );
    float metalnessTint = 1.0f;
    float roughnessTint = 1.0f;
    glm::vec3 emissiveTint = glm::vec3( 0 );
    
    // which textures from the textureset to actually apply
    bool applyAlbedo = true;
    bool applyMetalness = true;
    bool applyNormals = true;
    bool applyRoughness = true;
    bool applyEmissive = true;
};

struct GfxImage;

struct Material : public BaseAsset
{
    Material() = default;

    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    
    MaterialType type;
    glm::vec3 albedoTint = glm::vec3( 1.0f );
    float metalnessTint  = 1.0f;
    GfxImage* albedoMetalnessImage = nullptr;

    float roughnessTint = 1.0f;
    GfxImage* normalRoughnessImage = nullptr;

    glm::vec3 emissiveTint = glm::vec3( 0.0f );
    GfxImage* emissiveImage = nullptr;
};

} // namespace PG
