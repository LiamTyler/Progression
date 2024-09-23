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

// #include <ft2build.h>
// #include FT_FREETYPE_H

using namespace std;

struct GlyphData
{
    msdfgen::Shape shape;
    vec2 translation; // in em
    vec2 size;        // in em
    ivec2 roundedSizeInPixels;
    int rectId;
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
        LOG( "%u: %c", character, (char)character );
        GlyphData& data = glyphDatas.emplace_back();
        if ( !loadGlyph( data.shape, font, character, FONT_SCALING_EM_NORMALIZED ) )
        {
            LOG_ERR( "Failed to load glyph data for character '%u'", character );
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
    for ( int r = 0; r < atlasImg.height; ++r )
    {
        for ( int c = 0; c < atlasImg.width; ++c )
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

#define MSDF 0
    const int SIZE        = 64;
    const int PADDED_SIZE = SIZE + 16;
    RawImage2D img( PADDED_SIZE, PADDED_SIZE, ImageFormat::R8_G8_B8_A8_UNORM );

    if ( FreetypeHandle* ft = initializeFreetype() )
    {
        if ( FontHandle* font = loadFont( ft, PG_ASSET_DIR "fonts/arial.ttf" ) )
        {
            FontMetrics metrics;
            if ( !getFontMetrics( metrics, font, msdfgen::FONT_SCALING_NONE ) )
            {
                LOG_ERR( "Failed to get metrics" );
                return 0;
            }
            Shape shape;

            for ( char character = 33; character <= 126; ++character )
            {
                if ( !loadGlyph( shape, font, 'M', FONT_SCALING_EM_NORMALIZED ) )
                {
                    shape.normalize();
                    Shape::Bounds bounds = shape.getBounds();
                    Range range( 8.0 / SIZE );
                    double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
                    l += range.lower, b += range.lower;
                    r -= range.lower, t -= range.lower;
                    double vR = t - b;
                    double hR = r - l;
                    edgeColoringSimple( shape, 3.0 );
                    // edgeColoringInkTrap( shape, 3.0 );
#if MSDF
                    Bitmap<float, 3> msdf( SIZE, SIZE );
                    SDFTransformation t( Projection( SIZE, Vector2( 0.125, 0.125 ) ), Range( 0.125 ) );
                    generateMSDF( msdf, shape, t );
#else
                    Bitmap<float, 1> msdf( PADDED_SIZE, PADDED_SIZE );
                    Vector2 translate = Vector2( -l, -b );
                    generateSDF( msdf, shape, range, Vector2( SIZE, SIZE ), translate );
#endif

                    for ( int r = 0; r < msdf.height(); ++r )
                    {
                        for ( int c = 0; c < msdf.width(); ++c )
                        {
                            img.SetPixelFromFloat4( r, c, vec4( 0, 0, 0, 1 ) );
                            for ( int i = 0; i < ( MSDF ? 3 : 1 ); ++i )
                            {
                                float x = msdf( c, msdf.height() - r - 1 )[i];
                                img.SetPixelFromFloat( r, c, i, Saturate( x ) );
                            }
                        }
                    }

                    img.Save( PG_ROOT_DIR "font_msdf.png" );
                }
            }
            destroyFont( font );
        }
        deinitializeFreetype( ft );
    }

    return 0;
}
