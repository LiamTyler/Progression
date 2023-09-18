#pragma once

#include "asset/types/base_asset.hpp"
#include "asset/types/gfx_image.hpp"

namespace PG
{

struct TexturesetCreateInfo : public BaseAssetCreateInfo
{
    bool clampHorizontal = false;
    bool clampVertical = false;
    bool flipVertically = true;

    // only set defaults below when there MUST be a texture of that type (only assumed for diffuse + metalness so far)
    std::string albedoMap = "$white";
    std::string metalnessMap = "$default_metalness";
    Channel metalnessSourceChannel = Channel::R;
    float metalnessScale = 1.0f;

    std::string normalMap = "$default_normalmap";
    float slopeScale = 1.0f;
    bool normalMapIsYUp = true;
    std::string roughnessMap = "$default_roughness";
    Channel roughnessSourceChannel = Channel::R;
    bool invertRoughness = false; // if the source map is actually a gloss map
    float roughnessScale = 1.0f;

    std::string emissiveMap;

    std::string GetAlbedoMap( bool isApplied ) const;
    std::string GetMetalnessMap( bool isApplied ) const;
    std::string GetNormalMap( bool isApplied ) const;
    std::string GetRoughnessMap( bool isApplied ) const;
    std::string GetEmissiveMap( bool isApplied ) const;

    std::string GetAlbedoMetalnessImageName( bool applyAlbedo, bool applyMetalness ) const;
    std::string GetNormalRoughnessImageName( bool applyNormals, bool applyRoughness ) const;
};

} // namespace PG