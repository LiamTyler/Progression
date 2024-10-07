#pragma once

#include "asset/types/gfx_image.hpp"

namespace PG
{

static constexpr u32 FONT_FIRST_CHARACTER_CODE = 32;
static constexpr u32 FONT_LAST_CHARACTER_CODE  = 126;
static constexpr u32 FONT_TOTAL_CHARACTERS     = FONT_LAST_CHARACTER_CODE - FONT_FIRST_CHARACTER_CODE + 1;

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
    struct Metrics
    {
        float maxSignedDistanceRange;
        float fontSize; // the size of the glyphs baked into the atlas
        float emSize;
        float ascenderY;
        float descenderY;
        float lineHeight;
        float underlineY;
        float underlineThickness;
    };

    struct Glyph
    {
        vec2 planeMin;
        vec2 planeMax;
        vec2 uvMin; // top left corner
        vec2 uvMax; // bottom right corner
        f32 advance;
        u32 characterCode;
    };

    Font() = default;

    void Free() override;
    bool FastfileLoad( Serializer* serializer ) override;
    bool FastfileSave( Serializer* serializer ) const override;
    const Glyph& GetGlyph( char c ) const;

    GfxImage fontAtlasTexture;
    std::vector<Glyph> glyphs;
    Metrics metrics;
};

} // namespace PG
