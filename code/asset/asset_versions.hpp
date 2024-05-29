#pragma once

#include "shared/core_defines.hpp"
#include <cstdint>

// If order is changed, all fastfiles need to be rebuilt
enum AssetType : unsigned int
{
    ASSET_TYPE_GFX_IMAGE = 0,
    ASSET_TYPE_MATERIAL  = 1,
    ASSET_TYPE_SCRIPT    = 2,
    ASSET_TYPE_MODEL     = 3,
    ASSET_TYPE_SHADER    = 4,
    ASSET_TYPE_UI_LAYOUT = 5,
    ASSET_TYPE_PIPELINE  = 6,

    // put all assets that don't actually have a struct below.
    // Eg: types that exist in JSON but only have CreateInfo's that get used by other assets
    ASSET_TYPE_TEXTURESET = 7,

    ASSET_TYPE_COUNT
};

constexpr unsigned int g_assetVersions[] = {
    9,  // ASSET_TYPE_GFX_IMAGE, "New name serialization"
    10, // ASSET_TYPE_MATERIAL,  "New name serialization"
    1,  // ASSET_TYPE_SCRIPT,    "New name serialization"
    4,  // ASSET_TYPE_MODEL,     "Shorter mat name serialization"
    4,  // ASSET_TYPE_SHADER,    "New name serialization"
    8,  // ASSET_TYPE_UI_LAYOUT, "New name serialization"
    3,  // ASSET_TYPE_PIPELINE,  "New name serialization"

    // put all assets that don't actually have a struct below.
    // Note: this means that bumping the image versions doesn't do anything :(
    //       ex: changes to texturesets need to just bump the entire gfx_image version

    0, // ASSET_TYPE_TEXTURESET, "use slopeScale, metalness + roughness tints correctly"
};

constexpr uint32_t PG_FASTFILE_VERSION = 8 + ARRAY_SUM( g_assetVersions ); // mouse button down/up callbacks

inline const char* const g_assetNames[] = {
    "Image",      // ASSET_TYPE_GFX_IMAGE
    "Material",   // ASSET_TYPE_MATERIAL
    "Script",     // ASSET_TYPE_SCRIPT
    "Model",      // ASSET_TYPE_MODEL
    "Shader",     // ASSET_TYPE_SHADER
    "UILayout",   // ASSET_TYPE_UI_LAYOUT
    "Pipeline",   // ASSET_TYPE_PIPELINE
    "Textureset", // ASSET_TYPE_TEXTURESET
};
