#include "asset/types/font.hpp"
#include "image.hpp"
#include "shared/bitops.hpp"
#include "shared/filesystem.hpp"
#include "shared/logger.hpp"
#include "shared/lz4_compressor.hpp"
#include <iostream>

#include "msdfgen.h"
#include "msdfgen/ext/import-font.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

// using namespace msdfgen;
using namespace PG;
#include <ft2build.h>
#include FT_FREETYPE_H

double getFontCoordinateScale( const FT_Face& face, msdfgen::FontCoordinateScaling coordinateScaling )
{
    using namespace msdfgen;
    switch ( coordinateScaling )
    {
    case FONT_SCALING_NONE: return 1;
    case FONT_SCALING_EM_NORMALIZED: return 1. / ( face->units_per_EM ? face->units_per_EM : 1 );
    case FONT_SCALING_LEGACY: return MSDFGEN_LEGACY_FONT_COORDINATE_SCALE;
    }
    return 1;
}

using namespace std;

struct GlyphData
{
    char character;
    msdfgen::Shape shape;
    vec2 translation; // in em
    vec2 size;        // in em
    ivec2 roundedSizeInPixels;
    int rectId;
    msdfgen::GlyphIndex glyphIndex;
    std::vector<float> kernings;
};

bool CreateFontAtlas( const std::string& fontFilename, int glyphSize, float emRange )
{
    using namespace msdfgen;
    FreetypeHandle* ft = initializeFreetype();
    if ( !ft )
    {
        LOG_ERR( "Failed to initialize freetype" );
        return false;
    }
    FontHandle* font = loadFont( ft, fontFilename.c_str() );
    if ( !font )
    {
        LOG_ERR( "Failed to load font %s", fontFilename.c_str() );
        return false;
    }

    std::vector<GlyphData> glyphDatas;
    std::vector<stbrp_rect> rects;
    glyphDatas.reserve( 200 );
    Range msdfRange( emRange );

    i64 minAtlasPixelsNeeded = 0;
    const u32 firstChar      = 33;
    const u32 lastChar       = 126;
    for ( u32 character = firstChar; character <= lastChar; ++character )
    {
        GlyphData& data = glyphDatas.emplace_back();
        data.character  = (char)character;
        if ( !loadGlyph( data.shape, font, character, FONT_SCALING_EM_NORMALIZED ) )
        {
            LOG_ERR( "Failed to load glyph data for character '%u'", character );
            return false;
        }
        if ( !getGlyphIndex( data.glyphIndex, font, character ) )
        {
            LOG_ERR( "Failed to get glyph index" );
            return false;
        }

        data.shape.normalize();
        const double maxCornerAngle = 3.0;
        edgeColoringInkTrap( data.shape, maxCornerAngle );
        Shape::Bounds bounds = data.shape.getBounds();
        double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
        l += msdfRange.lower, b += msdfRange.lower;
        r -= msdfRange.lower, t -= msdfRange.lower;
        data.translation = vec2( -l, -b );
        data.size        = vec2( r - l, t - b );
        if ( data.size.x >= 2.0f || data.size.y >= 2.0f )
        {
            LOG_ERR( "Glyph is too large for rendering into bitmap! Size %f x %f", data.size.x, data.size.y );
            return false;
        }
        data.roundedSizeInPixels.x = static_cast<int>( ceil( data.size.x * glyphSize ) );
        data.roundedSizeInPixels.y = static_cast<int>( ceil( data.size.y * glyphSize ) );

        stbrp_rect& rect = rects.emplace_back();
        rect.id          = character - firstChar;
        rect.w           = data.roundedSizeInPixels.x;
        rect.h           = data.roundedSizeInPixels.y;
        minAtlasPixelsNeeded += static_cast<i64>( rect.w * rect.h );
        rect.x = rect.y = rect.was_packed = 0;
    }

    auto& A         = glyphDatas['A' - 33];
    auto& L         = glyphDatas['V' - 33];
    double kerning1 = 0;
    double kerning2 = 0;
    double kerning3 = 0;
    bool success    = getKerning( kerning1, font, A.glyphIndex, L.glyphIndex, msdfgen::FONT_SCALING_EM_NORMALIZED );
    success         = getKerning( kerning2, font, A.glyphIndex, L.glyphIndex, msdfgen::FONT_SCALING_LEGACY );
    success         = getKerning( kerning3, font, A.glyphIndex, L.glyphIndex, msdfgen::FONT_SCALING_NONE );

    // FontMetrics metrics;

    int width  = 64;
    int height = 64;
    while ( width * height < minAtlasPixelsNeeded )
    {
        if ( width == height )
            width *= 2;
        else
            height = width;
    }

    if ( width > 4096 || height > 4096 )
    {
        LOG_ERR( "Atlas needs over 4k x 4k texture size according to the packer. Retry with smaller sizes" );
        return false;
    }

    stbrp_context packingContext;
    int everythingPacked = 0;
    do
    {
        std::vector<stbrp_node> scratchNodes( width );
        stbrp_init_target( &packingContext, width, height, scratchNodes.data(), (int)scratchNodes.size() );
        everythingPacked = stbrp_pack_rects( &packingContext, rects.data(), (int)glyphDatas.size() );
        if ( !everythingPacked )
        {
            if ( width == height )
                width *= 2;
            else
                height = width;

            if ( width > 4096 || height > 4096 )
            {
                LOG_ERR( "Atlas needs over 4k x 4k texture size according to the packer. Retry with smaller sizes" );
                return false;
            }
        }
    } while ( !everythingPacked );

    for ( u64 i = 0; i < rects.size(); ++i )
    {
        glyphDatas[rects[i].id].rectId = (int)i;
    }

    RawImage2D atlasImg( width, height, ImageFormat::R8_G8_B8_A8_UNORM );
    for ( u32 r = 0; r < atlasImg.height; ++r )
    {
        for ( u32 c = 0; c < atlasImg.width; ++c )
        {
            atlasImg.SetPixelFromFloat4( r, c, vec4( 0, 0, 0, 1 ) );
        }
    }

    const int maxSize = 2 * glyphSize;
    Bitmap<float, 1> bitmap( maxSize, maxSize );
    for ( const GlyphData& glyph : glyphDatas )
    {
        Vector2 translation = Vector2( glyph.translation.x, glyph.translation.y );
        generateSDF( bitmap, glyph.shape, msdfRange, Vector2( glyphSize, glyphSize ), translation );

        const stbrp_rect& rect = rects[glyph.rectId];
        for ( int r = 0; r < rect.h; ++r )
        {
            for ( int c = 0; c < rect.w; ++c )
            {
                atlasImg.SetPixelFromFloat4( rect.y + r, rect.x + c, vec4( 0, 0, 0, 1 ) );
                for ( int i = 0; i < 1; ++i )
                {
                    float x = bitmap( c, rect.h - r - 1 )[i];
                    atlasImg.SetPixelFromFloat( rect.y + r, rect.x + c, i, Saturate( x ) );
                }
            }
        }
    }

    FT_Library library;
    FT_Error error = FT_Init_FreeType( &library );
    if ( error )
    {
        LOG_ERR( "Could not init freetype library" );
        return false;
    }

    FT_Face face;
    error = FT_New_Face( library, fontFilename.c_str(), 0, &face );

    double fontScale = getFontCoordinateScale( face, FONT_SCALING_EM_NORMALIZED );
    std::vector<Font::Glyph> pgGlyphs;
    for ( u32 character = FONT_FIRST_CHARACTER_CODE; character <= FONT_LAST_CHARACTER_CODE; ++character )
    {
        const GlyphData& data     = glyphDatas[character - FONT_FIRST_CHARACTER_CODE];
        const stbrp_rect& rect    = rects[character - FONT_FIRST_CHARACTER_CODE];
        Font::Glyph& pgGlyph      = pgGlyphs.emplace_back();
        pgGlyph.characterCode     = character;
        pgGlyph.positionInAtlas.x = rect.x / (float)atlasImg.width;
        pgGlyph.positionInAtlas.y = rect.y / (float)atlasImg.height;
        pgGlyph.sizeInAtlas.x     = rect.w / (float)atlasImg.width;
        pgGlyph.sizeInAtlas.y     = rect.h / (float)atlasImg.height;

        // FT_Error error = FT_Load_Glyph( font->face, data.glyphIndex, FT_LOAD_NO_SCALE );
        FT_Error error = FT_Load_Glyph( face, data.glyphIndex.getIndex(), FT_LOAD_DEFAULT );
        if ( error )
            return false;

        pgGlyph.advance   = static_cast<float>( fontScale * face->glyph->advance.x );
        pgGlyph.bearing.x = static_cast<float>( fontScale * face->glyph->metrics.horiBearingX );
        pgGlyph.bearing.y = static_cast<float>( fontScale * face->glyph->metrics.horiBearingY );
    }

    atlasImg.Save( PG_ROOT_DIR "font_atlas.png" );

    return true;
}

using namespace msdfgen;
int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    if ( !CreateFontAtlas( PG_ASSET_DIR "fonts/arial.ttf", 32, 0.125 ) )
    {
        LOG_ERR( "Could not create atlas" );
        return 0;
    }

    return 0;
}
