#pragma once

namespace PG
{

constexpr int PG_FASTFILE_VERSION = 3; // Material saving fixed

constexpr int PG_GFX_IMAGE_VERSION = 2; // adding flipVerticallyOption

constexpr int PG_MATERIAL_VERSION = 3; // adding material types

constexpr int PG_MODEL_VERSION = 3; // adding header info for models

constexpr int PG_SCRIPT_VERSION = 1; // initial version

constexpr int PG_SHADER_VERSION = 3; // adding debug info for non-optimized shaders


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

} // namespace PG