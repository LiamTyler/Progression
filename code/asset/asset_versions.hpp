#pragma once

#include "shared/core_defines.hpp"

// If order is changed, all fastfiles need to be rebuilt
enum AssetType : u8
{
    ASSET_TYPE_GFX_IMAGE = 0,
    ASSET_TYPE_MATERIAL  = 1,
    ASSET_TYPE_SCRIPT    = 2,
    ASSET_TYPE_MODEL     = 3,
    ASSET_TYPE_SHADER    = 4,
    ASSET_TYPE_PIPELINE  = 5,
    ASSET_TYPE_FONT      = 6,

    ASSET_TYPE_NON_METADATA_COUNT = 7, // like ASSET_TYPE_COUNT, but only for assets that actually produce .ffi files
    // put all assets that don't actually have a struct below.
    // Eg: types that exist in JSON but only have CreateInfo's that get used by other assets
    ASSET_TYPE_TEXTURESET = 7,

    ASSET_TYPE_COUNT
};

constexpr i32 g_globalAssetVersion = 1; // "New asset metadata"

constexpr i32 g_assetVersions[] = {
    g_globalAssetVersion + 9,  // ASSET_TYPE_GFX_IMAGE, "New name serialization"
    g_globalAssetVersion + 10, // ASSET_TYPE_MATERIAL,  "New name serialization"
    g_globalAssetVersion + 1,  // ASSET_TYPE_SCRIPT,    "New name serialization"
    g_globalAssetVersion + 19, // ASSET_TYPE_MODEL,     "Packed UVs"
    g_globalAssetVersion + 4,  // ASSET_TYPE_SHADER,    "New name serialization"
    g_globalAssetVersion + 5,  // ASSET_TYPE_PIPELINE,  "Forgot to add new blend mode to the cache name"
    g_globalAssetVersion + 5,  // ASSET_TYPE_FONT,      "Kerning + switched the edge coloring mode"

    // put all assets that don't actually have a struct below.
    // Note: this means that bumping the textureset versions doesn't do anything :(
    //       ex: changes to texturesets need to just bump the entire gfx_image version

    0, // ASSET_TYPE_TEXTURESET, "use slopeScale, metalness + roughness tints correctly"
};

constexpr u32 PG_FASTFILE_VERSION = 20 + ARRAY_SUM( g_assetVersions ); // removing ui layout asset

inline const char* const g_assetNames[] = {
    "Image",      // ASSET_TYPE_GFX_IMAGE
    "Material",   // ASSET_TYPE_MATERIAL
    "Script",     // ASSET_TYPE_SCRIPT
    "Model",      // ASSET_TYPE_MODEL
    "Shader",     // ASSET_TYPE_SHADER
    "Pipeline",   // ASSET_TYPE_PIPELINE
    "Font",       // ASSET_TYPE_FONT
    "Textureset", // ASSET_TYPE_TEXTURESET
};
