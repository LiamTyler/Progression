#pragma once

#include "asset/types/base_asset.hpp"

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
    bool clampU = false;
    bool clampV = false;

    std::string albedoMap;
    std::string metalnessMap;
    ChannelSelect metalnessSourceChannel = ChannelSelect::R;
    float metalnessScale = 1.0f;

    std::string normalMap;
    float slopeScale = 1.0f;
    std::string roughnessMap;
    ChannelSelect roughnessSourceChannel = ChannelSelect::R;
    bool invertRoughness = false; // if the source map is actually a gloss map

    std::string GetAlbedoMetalnessImageName( bool applyAlbedo, bool applyMetalness ) const;
    std::string GetNormalRoughImageName( bool applyNormals, bool applyRoughness ) const;
};

} // namespace PG