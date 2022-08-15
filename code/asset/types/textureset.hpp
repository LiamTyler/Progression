#pragma once

#include "asset/types/base_asset.hpp"
#include "asset/types/gfx_image.hpp"

namespace PG
{

struct TexturesetCreateInfo : public BaseAssetCreateInfo
{
    bool clampHorizontal = false;
    bool clampVertical = false;

    std::string albedoMap = "$white";
    std::string metalnessMap = "$default_metalness";
    Channel metalnessSourceChannel = Channel::R;
    float metalnessScale = 1.0f;

    //std::string normalMap = "$default_normalmap";
    //std::string roughnessMap = "$default_roughness";
    //Channel roughnessSourceChannel = Channel::R;
    //bool invertRoughness = false; // if the source map is actually a gloss map
    //float roughnessScale = 1.0f;

    std::string GetAlbedoMap( bool isApplied ) const;
    std::string GetMetalnessMap( bool isApplied ) const;
    //std::string GetNormalMap( bool isApplied ) const;
    //std::string GetRoughnessMap( bool isApplied ) const;

    std::string GetAlbedoMetalnessImageName( bool applyAlbedo, bool applyMetalness ) const;
    //std::string GetNormalRoughImageName( bool applyNormals, bool applyRoughness ) const;
};

} // namespace PG