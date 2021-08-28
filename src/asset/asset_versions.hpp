#pragma once

constexpr int PG_FASTFILE_VERSION = 4; // Adding metalness and roughness maps + tints

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


inline unsigned int g_assetVersions[] =
{
    0, // ASSET_TYPE_GFX_IMAGE
    0, // ASSET_TYPE_MATERIAL
    0, // ASSET_TYPE_SCRIPT
    0, // ASSET_TYPE_MODEL
    0, // ASSET_TYPE_SHADER
};