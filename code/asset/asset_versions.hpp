#pragma once

constexpr int PG_FASTFILE_VERSION = 5; // different material serialization

// If order is changed, all fastfiles need to be rebuilt
enum AssetType : unsigned int
{
    ASSET_TYPE_GFX_IMAGE = 0,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_SCRIPT,
    ASSET_TYPE_MODEL,
    ASSET_TYPE_SHADER,

    NUM_ASSET_TYPES
};


inline const unsigned int g_assetVersions[] =
{
    0, // ASSET_TYPE_GFX_IMAGE
    0, // ASSET_TYPE_MATERIAL
    0, // ASSET_TYPE_SCRIPT
    0, // ASSET_TYPE_MODEL
    0, // ASSET_TYPE_SHADER
};

inline const char* const g_assetNames[] =
{
    "Image",    // ASSET_TYPE_GFX_IMAGE
    "Material", // ASSET_TYPE_MATERIAL
    "Script",   // ASSET_TYPE_SCRIPT
    "Model",    // ASSET_TYPE_MODEL
    "Shader",   // ASSET_TYPE_SHADER
};