#pragma once

constexpr int PG_FASTFILE_VERSION = 4; // Adding metalness and roughness maps + tints

constexpr int PG_GFX_IMAGE_VERSION = 2; // adding flipVerticallyOption

constexpr int PG_MATERIAL_VERSION = 1; // Adding metalness and roughness maps + tints

constexpr int PG_MODEL_VERSION = 3; // fixed bug in RecalculateNormals, where an edge was (0,0,0), causing NaNs

constexpr int PG_SCRIPT_VERSION = 1; // initial version

constexpr int PG_SHADER_VERSION = 3; // adding debug info for non-optimized shaders

constexpr int PG_ENVIRONMENT_MAP_VERSION = 1; // initial version


// If order is changed, all fastfiles need to be rebuilt
enum AssetType : unsigned int
{
    ASSET_TYPE_GFX_IMAGE = 0,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_SCRIPT,
    ASSET_TYPE_MODEL,
    ASSET_TYPE_SHADER,
    ASSET_TYPE_ENVIRONMENTMAP,

    NUM_ASSET_TYPES
};