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
    ivec2 atlasSize;
    ivec2 atlasPos;
    std::vector<float> kernings;

    vec2 translation; // in em
    vec2 size;        // in em
    float scale;
    int rectId;
    msdfgen::GlyphIndex glyphIndex;
    msdfgen::Shape shape;

    vec4 getQuadAtlasBounds() const
    {
        vec4 ret( 0 );
        if ( atlasSize.x > 0 && atlasSize.y > 0 )
        {
            float invScale = 1.0f / scale;
            ret[0]         = ( -translation.x + ( .5f ) * invScale );
            ret[2]         = ( -translation.y + ( .5f ) * invScale );
            ret[1]         = ( -translation.x + ( atlasSize.x - .5f ) * invScale );
            ret[3]         = ( -translation.y + ( atlasSize.y - .5f ) * invScale );
        }

        return ret;
    }
};

bool PackAtlas( std::vector<GlyphData>& glyphs, int& width, int& height )
{
    i64 minAtlasPixelsNeeded = 0;
    std::vector<stbrp_rect> rects;
    for ( size_t i = 0; i < glyphs.size(); ++i )
    {
        const GlyphData& glyph = glyphs[i];
        stbrp_rect& rect       = rects.emplace_back();
        rect.id                = (int)i;
        rect.w                 = glyph.atlasSize.x;
        rect.h                 = glyph.atlasSize.y;
        rect.x = rect.y = rect.was_packed = 0;

        minAtlasPixelsNeeded += static_cast<i64>( rect.w * rect.h );
    }

    width  = 64;
    height = 64;
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
        everythingPacked = stbrp_pack_rects( &packingContext, rects.data(), (int)rects.size() );
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

    for ( size_t i = 0; i < rects.size(); ++i )
    {
        const stbrp_rect& rect = rects[i];
        GlyphData& glyph       = glyphs[rect.id];
        glyph.atlasPos.x       = rect.x;
        glyph.atlasPos.y       = rect.y;
    }

    return true;
}

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
    glyphDatas.reserve( 200 );
    Range msdfRange( emRange );

    bool pxAlignOriginX = true;
    bool pxAlignOriginY = true;

    const u32 firstChar = 'g';
    const u32 lastChar  = 'g';
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

        float scale = (float)glyphSize;
        data.scale  = scale;
        if ( pxAlignOriginX )
        {
            int sl             = (int)floor( scale * l - .5 );
            int sr             = (int)ceil( scale * r + .5 );
            data.atlasSize.x   = sr - sl;
            data.translation.x = -sl / scale;
        }
        else
        {
            double w           = scale * ( r - l );
            data.atlasSize.x   = (int)ceil( w ) + 1;
            data.translation.x = static_cast<float>( -l + .5 * ( data.atlasSize.x - w ) / scale );
        }

        if ( pxAlignOriginY )
        {
            int sb             = (int)floor( scale * b - .5 );
            int st             = (int)ceil( scale * t + .5 );
            data.atlasSize.y   = st - sb;
            data.translation.y = -sb / scale;
        }
        else
        {
            double h           = scale * ( t - b );
            data.atlasSize.y   = (int)ceil( h ) + 1;
            data.translation.y = static_cast<float>( -b + .5 * ( data.atlasSize.y - h ) / scale );
        }

        if ( data.atlasSize.x >= ( 2 * glyphSize ) || data.atlasSize.y >= ( 2 * glyphSize ) )
        {
            LOG_ERR( "Glyph is too large for rendering into bitmap! Size %d x %d", data.atlasSize.x, data.atlasSize.y );
            return false;
        }
    }

    // auto& A         = glyphDatas['A' - 33];
    // auto& L         = glyphDatas['V' - 33];
    // double kerning1 = 0;
    // double kerning2 = 0;
    // double kerning3 = 0;
    // bool success    = getKerning( kerning1, font, A.glyphIndex, L.glyphIndex, msdfgen::FONT_SCALING_EM_NORMALIZED );
    // success         = getKerning( kerning2, font, A.glyphIndex, L.glyphIndex, msdfgen::FONT_SCALING_LEGACY );
    // success         = getKerning( kerning3, font, A.glyphIndex, L.glyphIndex, msdfgen::FONT_SCALING_NONE );

    // FontMetrics metrics;

    int width, height;
    if ( !PackAtlas( glyphDatas, width, height ) )
    {
        LOG_ERR( "Failed to pack all glyphs into an atlas!" );
        return false;
    }

    RawImage2D atlasImg( width, height, ImageFormat::R8_G8_B8_A8_UNORM );
    for ( u32 r = 0; r < atlasImg.height; ++r )
    {
        for ( u32 c = 0; c < atlasImg.width; ++c )
        {
            atlasImg.SetPixelFromFloat4( r, c, vec4( 0, 0, 0, 1 ) );
        }
    }

    // TODO: group into 1 config struct, alone with AlignX and alignY
    bool scanlinePass                   = true;
    msdfgen::MSDFGeneratorConfig config = {};
    config.overlapSupport               = true;
    //errorCorrectionModes (for msdf and mtsdf)

    const int maxSize = 2 * glyphSize;
    msdfgen::Bitmap<float, 1> bitmap( maxSize, maxSize );
    for ( const GlyphData& glyph : glyphDatas )
    {
        msdfgen::Vector2 scale     = msdfgen::Vector2( glyph.scale );
        msdfgen::Vector2 translate = msdfgen::Vector2( glyph.translation.x, glyph.translation.y );
        msdfgen::Projection proj( scale, translate );
        msdfgen::generateSDF( bitmap, glyph.shape, proj, msdfRange, config );
        if ( scanlinePass )
            msdfgen::distanceSignCorrection( bitmap, glyph.shape, proj, msdfgen::FILL_NONZERO );

        for ( int r = 0; r < glyph.atlasSize.y; ++r )
        {
            for ( int c = 0; c < glyph.atlasSize.x; ++c )
            {
                atlasImg.SetPixelFromFloat4( glyph.atlasPos.y + r, glyph.atlasPos.x + c, vec4( 0, 0, 0, 1 ) );
                for ( int i = 0; i < 1; ++i )
                {
                    float x = bitmap( c, glyph.atlasSize.y - r - 1 )[i];
                    atlasImg.SetPixelFromFloat( glyph.atlasPos.y + r, glyph.atlasPos.x + c, i, Saturate( x ) );
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
    for ( size_t i = 0; i < glyphDatas.size(); ++i )
    {
        // const GlyphData& data     = glyphDatas[character - FONT_FIRST_CHARACTER_CODE];
        const GlyphData& data     = glyphDatas[i];
        Font::Glyph& pgGlyph      = pgGlyphs.emplace_back();
        pgGlyph.characterCode     = data.character;
        pgGlyph.positionInAtlas.x = data.atlasPos.x / (float)atlasImg.width;
        pgGlyph.positionInAtlas.y = data.atlasPos.y / (float)atlasImg.height;
        pgGlyph.sizeInAtlas.x     = data.atlasSize.x / (float)atlasImg.width;
        pgGlyph.sizeInAtlas.y     = data.atlasSize.y / (float)atlasImg.height;

        // FT_Error error = FT_Load_Glyph( font->face, data.glyphIndex, FT_LOAD_NO_SCALE );
        FT_Error error = FT_Load_Glyph( face, data.glyphIndex.getIndex(), FT_LOAD_DEFAULT );
        if ( error )
            return false;

        pgGlyph.advance   = static_cast<float>( fontScale * face->glyph->advance.x );
        pgGlyph.bearing.x = static_cast<float>( fontScale * face->glyph->metrics.horiBearingX );
        pgGlyph.bearing.y = static_cast<float>( fontScale * face->glyph->metrics.horiBearingY );

        vec4 planeBounds = data.getQuadAtlasBounds();
        LOG( "unicode: %d", (int)data.character );
        LOG( "advance: %f", pgGlyph.advance );
        LOG( "planeBounds: {" );
        LOG( "  \"left\": %f", planeBounds[0] );
        LOG( "  \"bottom\": %f", planeBounds[2] );
        LOG( "  \"right\": %f", planeBounds[1] );
        LOG( "  \"top\": %f", planeBounds[3] );
        LOG( "}," );
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
