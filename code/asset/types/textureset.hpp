#pragma once

#include "asset/types/base_asset.hpp"
#include "asset/types/gfx_image.hpp"

namespace PG
{

enum class ChannelSelect : uint8_t
{
    R = 0,
    G = 1,
    B = 2,
    A = 3,
};

struct TexturesetCreateInfo : public BaseAssetCreateInfo
{
    std::string albedoMap;

    std::string metalnessMap;
    ChannelSelect metalnessSourceChannel = ChannelSelect::R;
    float metalnessScale = 1.0f;

    std::string normalMap;
    float slopeScale = 1.0f;

    std::string roughnessMap;
    ChannelSelect roughnessSourceChannel = ChannelSelect::R;
    bool invertRoughness = false; // if the source map is actually a gloss map
};

struct Textureset : public BaseAsset
{
public:
    Textureset() = default;

    bool Load( const BaseAssetCreateInfo* baseInfo ) override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    GfxImage *albedoMetalTex; // albedo RGB, metal A
    GfxImage *normalRoughTex; // normal XY, roughness B
};

} // namespace PG