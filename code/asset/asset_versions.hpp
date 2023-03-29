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

    // put all assets that don't actually have a struct below.
    // Eg: types that exist in JSON but only have CreateInfo's that get used by other assets
    ASSET_TYPE_TEXTURESET = 6,

    ASSET_TYPE_COUNT
};


constexpr unsigned int g_assetVersions[] =
{
    3, // ASSET_TYPE_GFX_IMAGE
    2, // ASSET_TYPE_MATERIAL
    0, // ASSET_TYPE_SCRIPT
    1, // ASSET_TYPE_MODEL
    0, // ASSET_TYPE_SHADER
    7, // ASSET_TYPE_UI_LAYOUT, "mouse button down/up callbacks"
    0, // ASSET_TYPE_TEXTURESET
};

constexpr uint32_t PG_FASTFILE_VERSION = 7 + ARRAY_SUM( g_assetVersions ); // mouse button down/up callbacks

inline const char* const g_assetNames[] =
{
    "Image",      // ASSET_TYPE_GFX_IMAGE
    "Material",   // ASSET_TYPE_MATERIAL
    "Script",     // ASSET_TYPE_SCRIPT
    "Model",      // ASSET_TYPE_MODEL
    "Shader",     // ASSET_TYPE_SHADER
    "UILayout",   // ASSET_TYPE_UI_LAYOUT
    "Textureset", // ASSET_TYPE_TEXTURESET
};
