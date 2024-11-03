#pragma once

#include "asset/types/base_asset.hpp"
#include "shared/math_vec.hpp"

class Serializer;

namespace PG
{

enum class MaterialType : u8
{
    SURFACE = 0,
    DECAL   = 1,

    COUNT
};

struct MaterialCreateInfo : public BaseAssetCreateInfo
{
    MaterialType type = MaterialType::SURFACE;
    std::string texturesetName;
    vec3 albedoTint   = vec3( 1 );
    f32 metalnessTint = 1.0f;
    f32 roughnessTint = 1.0f;
    vec3 emissiveTint = vec3( 0 );

    // which textures from the textureset to actually apply
    bool applyAlbedo    = true;
    bool applyMetalness = true;
    bool applyNormals   = true;
    bool applyRoughness = true;
    bool applyEmissive  = true;
};

struct GfxImage;

struct Material : public BaseAsset
{
    Material();
    ~Material();

    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    void Free() override;

    MaterialType type;
    vec3 albedoTint                = vec3( 1.0f );
    f32 metalnessTint              = 1.0f;
    GfxImage* albedoMetalnessImage = nullptr;

    f32 roughnessTint              = 1.0f;
    GfxImage* normalRoughnessImage = nullptr;

    vec3 emissiveTint       = vec3( 0.0f );
    GfxImage* emissiveImage = nullptr;

    u16 GetBindlessIndex() const { return m_bindlessIndex; }

private:
    u16 m_bindlessIndex;
};

} // namespace PG
