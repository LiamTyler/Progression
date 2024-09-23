#pragma once

#include "asset/types/gfx_image.hpp"

namespace PG
{

struct FontCreateInfo : public BaseAssetCreateInfo
{
    std::string filename;
    int glyphSize           = 32;     // in pixels
    float maxSignedDistance = 0.125f; // in em units
};

std::string GetAbsPath_FontFilename( const std::string& filename );

struct Font : public BaseAsset
{
public:
    Font() = default;

    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    GfxImage* fontAtlasTexture;
};

} // namespace PG
