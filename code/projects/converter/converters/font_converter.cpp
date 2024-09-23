#include "font_converter.hpp"
#include "shared/hash.hpp"

#include "msdfgen.h"
#include "msdfgen/ext/import-font.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"
#include <ft2build.h>
#include FT_FREETYPE_H

namespace PG
{

std::string FontConverter::GetCacheNameInternal( ConstDerivedInfoPtr info )
{
    std::string cacheName = info->name;
    size_t hash           = 0;
    HashCombine( hash, info->filename );
    HashCombine( hash, info->glyphSize );
    HashCombine( hash, info->maxSignedDistance );
    cacheName += "_" + std::to_string( hash );
    cacheName += "_" + std::to_string( g_assetVersions[ASSET_TYPE_GFX_IMAGE] );
    return cacheName;
}

AssetStatus FontConverter::IsAssetOutOfDateInternal( ConstDerivedInfoPtr info, time_t cacheTimestamp )
{
    const std::string absFilename = GetAbsPath_FontFilename( info->filename );
    AddFastfileDependency( absFilename );
    if ( IsFileOutOfDate( cacheTimestamp, absFilename ) )
        return AssetStatus::OUT_OF_DATE;

    // std::string outputTexCacheName = "#font_" + GetCacheNameInternal( info ) + std::to_string( g_assetVersions[ASSET_TYPE_GFX_IMAGE] );
    // time_t texTimestamp = AssetCache::GetAssetTimestamp( ASSET_TYPE_GFX_IMAGE, outputTexCacheName );
    // if ( IsFileOutOfDate( cacheTimestamp, texTimestamp ) )
    //     return AssetStatus::OUT_OF_DATE;
    //
    return AssetStatus::UP_TO_DATE;
}

struct GlyphData
{
    msdfgen::Shape shape;
    vec2 translation; // in em
    vec2 size;        // in em
    ivec2 roundedSizeInPixels;
    int rectId;
};

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

bool CreateFontAtlas( const FontCreateInfo& info, Font& fontAsset, RawImage2D& atlasImg )
{
    using namespace msdfgen;
    FreetypeHandle* ft = initializeFreetype();
    if ( !ft )
    {
        LOG_ERR( "Failed to initialize freetype" );
        return false;
    }
    FontHandle* font = loadFont( ft, GetAbsPath_FontFilename( info.filename ).c_str() );
    if ( !font )
    {
        LOG_ERR( "Failed to load font %s", GetAbsPath_FontFilename( info.filename ).c_str() );
        return false;
    }

    const u32 TOTAL_CHARS = FONT_LAST_CHARACTER_CODE - FONT_FIRST_CHARACTER_CODE + 1;
    std::vector<GlyphData> glyphDatas;
    std::vector<stbrp_rect> rects;
    glyphDatas.reserve( TOTAL_CHARS );
    Range msdfRange( info.maxSignedDistance );

    i64 minAtlasPixelsNeeded = 0;
    for ( u32 character = FONT_FIRST_CHARACTER_CODE; character <= FONT_LAST_CHARACTER_CODE; ++character )
    {
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
        data.roundedSizeInPixels.x = static_cast<int>( ceil( data.size.x * info.glyphSize ) );
        data.roundedSizeInPixels.y = static_cast<int>( ceil( data.size.y * info.glyphSize ) );

        stbrp_rect& rect = rects.emplace_back();
        rect.id          = character - FONT_FIRST_CHARACTER_CODE;
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

    atlasImg = RawImage2D( width, height, ImageFormat::R8_G8_B8_A8_UNORM );
    for ( u32 r = 0; r < atlasImg.height; ++r )
    {
        for ( u32 c = 0; c < atlasImg.width; ++c )
        {
            atlasImg.SetPixelFromFloat4( r, c, vec4( 0, 0, 0, 1 ) );
        }
    }

    const int maxSize = 2 * info.glyphSize;
    Bitmap<float, 1> bitmap( maxSize, maxSize );
    for ( const GlyphData& glyph : glyphDatas )
    {
        Vector2 translation = Vector2( glyph.translation.x, glyph.translation.y );
        generateSDF( bitmap, glyph.shape, msdfRange, Vector2( info.glyphSize, info.glyphSize ), translation );

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
    error = FT_New_Face( library, GetAbsPath_FontFilename( info.filename ).c_str(), 0, &face );
    if ( error )
    {
        LOG_ERR( "Could not load font with freetype" );
        return false;
    }

    double fontScale = getFontCoordinateScale( face, FONT_SCALING_EM_NORMALIZED );
    for ( u32 character = FONT_FIRST_CHARACTER_CODE; character <= FONT_LAST_CHARACTER_CODE; ++character )
    {
        const GlyphData& data     = glyphDatas[character - FONT_FIRST_CHARACTER_CODE];
        const stbrp_rect& rect    = rects[character - FONT_FIRST_CHARACTER_CODE];
        Font::Glyph& pgGlyph      = fontAsset.glyphs.emplace_back();
        pgGlyph.characterCode     = character;
        pgGlyph.positionInAtlas.x = rect.x / (float)atlasImg.width;
        pgGlyph.positionInAtlas.y = rect.y / (float)atlasImg.height;
        pgGlyph.sizeInAtlas.x     = data.size.x / (float)atlasImg.width;
        pgGlyph.sizeInAtlas.y     = data.size.y / (float)atlasImg.height;

        FT_Error error = FT_Load_Glyph( face, FT_Get_Char_Index( face, character ), FT_LOAD_DEFAULT );
        if ( error )
            return false;

        pgGlyph.bearing.x = static_cast<float>( fontScale * face->glyph->metrics.horiBearingX );
        pgGlyph.bearing.y = static_cast<float>( fontScale * face->glyph->metrics.horiBearingY );
        pgGlyph.advance   = static_cast<float>( fontScale * face->glyph->advance.x );
    }

    FT_Done_Face( face );
    FT_Done_FreeType( library );

    return true;
}

bool FontConverter::ConvertInternal( ConstDerivedInfoPtr& info )
{
    Font font;
    font.SetName( info->name );

    RawImage2D atlasImg;
    if ( !CreateFontAtlas( *info, font, atlasImg ) )
    {
        LOG_ERR( "Could not create font atlas for font '%s'", info->name.c_str() );
        return false;
    }

    GfxImage gfxImage;
    RawImage2DMipsToGfxImage( gfxImage, { atlasImg }, PixelFormat::R8_G8_B8_A8_UNORM );
    gfxImage.SetName( "font_atlas_" + info->name );

    std::string cacheName = GetCacheName( info );
    if ( !AssetCache::CacheAsset( assetType, cacheName, &font ) )
    {
        LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], info->name.c_str(), cacheName.c_str() );
        return false;
    }

    return true;
}

} // namespace PG
