#pragma once

#include "asset/types/gfx_image.hpp"

namespace PG
{

static constexpr u32 FONT_FIRST_CHARACTER_CODE = 33;
static constexpr u32 FONT_LAST_CHARACTER_CODE  = 126;

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
    struct Glyph
    {
        vec2 positionInAtlas;
        vec2 sizeInAtlas;
        vec2 bearing;
        f32 advance;
        u32 characterCode;
    };

    Font() = default;

    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;

    GfxImage fontAtlasTexture;
    std::vector<Glyph> glyphs;
};

} // namespace PG
