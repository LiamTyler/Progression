#pragma once

constexpr int PG_FASTFILE_VERSION = 2; // shader reflection data

constexpr int PG_GFX_IMAGE_VERSION = 1; // initial version

constexpr int PG_MATERIAL_VERSION = 1; // initial version

constexpr int PG_MODEL_VERSION = 1; // initial version

constexpr int PG_SCRIPT_VERSION = 1; // initial version

constexpr int PG_SHADER_VERSION = 2; // reflection data


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