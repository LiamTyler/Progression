#pragma once

constexpr int PG_FASTFILE_VERSION = 1; // initial version

constexpr int PG_GFX_IMAGE_VERSION = 1; // initial version

constexpr int PG_MATERIAL_VERSION = 1; // initial version

constexpr int PG_SCRIPT_VERSION = 1; // initial version

enum AssetType : unsigned int
{
    ASSET_TYPE_GFX_IMAGE = 0,
    ASSET_TYPE_MATERIAL,
    ASSET_TYPE_SCRIPT,
    NUM_ASSET_TYPES
};