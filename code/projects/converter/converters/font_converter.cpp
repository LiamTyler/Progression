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

#define FONT_Y_DOWN 1

struct Config
{
    Config()
    {
        glyphSize              = 32;
        emRange                = 0.125f;
        pxAlignOriginX         = true;
        pxAlignOriginY         = true;
        scanlinePass           = true;
        gconfig                = {};
        gconfig.overlapSupport = true;

        gconfig.errorCorrection.mode              = msdfgen::ErrorCorrectionConfig::EDGE_PRIORITY;
        gconfig.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
    }

    int glyphSize;
    float emRange;
    bool pxAlignOriginX;
    bool pxAlignOriginY;
    bool scanlinePass;
    msdfgen::MSDFGeneratorConfig gconfig;
};

struct GlyphData
{
    char character;
    ivec2 atlasSize;
    ivec2 atlasPos;
    std::vector<float> kernings;

    vec2 translation; // in em
    vec2 size;        // in em
    float scale;
    msdfgen::GlyphIndex glyphIndex;
    msdfgen::Shape shape;

    // GlyphGeometry.cpp::GlyphGeometry::getQuadPlaneBounds()
    void getQuadAtlasBounds( float& l, float& b, float& r, float& t ) const
    {
        l = b = r = t = 0;
        if ( atlasSize.x > 0 && atlasSize.y > 0 )
        {
            float invScale = 1.0f / scale;
            l              = ( -translation.x + ( .5f ) * invScale );
            b              = ( -translation.y + ( .5f ) * invScale );
            r              = ( -translation.x + ( atlasSize.x - .5f ) * invScale );
            t              = ( -translation.y + ( atlasSize.y - .5f ) * invScale );
        }
    }
};

bool IsWhiteSpace( char c ) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

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

bool PackAtlas( std::vector<GlyphData>& glyphs, int& width, int& height )
{
    i64 minAtlasPixelsNeeded = 0;
    std::vector<stbrp_rect> rects;
    for ( size_t i = 0; i < glyphs.size(); ++i )
    {
        const GlyphData& glyph = glyphs[i];
        if ( IsWhiteSpace( glyph.character ) )
            continue;

        stbrp_rect& rect = rects.emplace_back();
        rect.id          = (int)i;
        rect.w           = glyph.atlasSize.x;
        rect.h           = glyph.atlasSize.y;
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

bool GetFontMetrics(
    const std::string& fontFilename, const std::vector<GlyphData>& glyphDatas, const RawImage2D& atlasImg, Font& fontAsset )
{
    FT_Library library;
    FT_Error error = FT_Init_FreeType( &library );
    if ( error )
    {
        LOG_ERR( "Could not init freetype library" );
        return false;
    }

    FT_Face face;
    error = FT_New_Face( library, GetAbsPath_FontFilename( fontFilename ).c_str(), 0, &face );

    double fontScale = getFontCoordinateScale( face, msdfgen::FONT_SCALING_EM_NORMALIZED );

    fontAsset.metrics.emSize             = static_cast<float>( fontScale * face->units_per_EM );
    fontAsset.metrics.emSize             = static_cast<float>( fontScale * face->units_per_EM );
    fontAsset.metrics.lineHeight         = static_cast<float>( fontScale * face->height );
    fontAsset.metrics.underlineThickness = static_cast<float>( fontScale * face->underline_thickness );
    fontAsset.metrics.underlineY         = static_cast<float>( fontScale * face->underline_position );
    fontAsset.metrics.ascenderY          = static_cast<float>( fontScale * face->ascender );
    fontAsset.metrics.descenderY         = static_cast<float>( fontScale * face->descender );

    fontAsset.glyphs.reserve( glyphDatas.size() );
    for ( size_t i = 0; i < glyphDatas.size(); ++i )
    {
        const GlyphData& data   = glyphDatas[i];
        Font::Glyph& glyphAsset = fontAsset.glyphs.emplace_back();

        float planeL, planeB, planeR, planeT;
        data.getQuadAtlasBounds( planeL, planeB, planeR, planeT );
        glyphAsset.planeMin = { planeL, FONT_Y_DOWN ? -planeT : planeB };
        glyphAsset.planeMax = { planeR, FONT_Y_DOWN ? -planeB : planeT };
        glyphAsset.uvMin.x  = ( data.atlasPos.x + 0.5f ) / (float)atlasImg.width;
        glyphAsset.uvMin.y  = ( data.atlasPos.y + 0.5f ) / (float)atlasImg.height;
        glyphAsset.uvMax.x  = ( data.atlasPos.x + data.atlasSize.x - 0.5f ) / (float)atlasImg.width;
        glyphAsset.uvMax.y  = ( data.atlasPos.y + data.atlasSize.y - 0.5f ) / (float)atlasImg.height;

        if ( data.character == 'l' )
        {
            LOG( "left: %f", glyphAsset.planeMin.x );
            LOG( "bottom: %f", glyphAsset.planeMin.y );
            LOG( "right: %f", glyphAsset.planeMax.x );
            LOG( "top: %f", glyphAsset.planeMax.y );
        }

        u32 glyphIndex = FT_Get_Char_Index( face, data.character );
        if ( !glyphIndex )
        {
            LOG_ERR( "Failed to get glyph index for character %u", glyphIndex );
            FT_Done_Face( face );
            FT_Done_FreeType( library );
            return false;
        }
        FT_Error error = FT_Load_Glyph( face, glyphIndex, FT_LOAD_DEFAULT );
        if ( error )
            return false;

        glyphAsset.advance       = static_cast<float>( fontScale * face->glyph->advance.x );
        glyphAsset.characterCode = data.character;
    }

    FT_Done_Face( face );
    FT_Done_FreeType( library );

    return true;
}

#define MTSDF IN_USE

bool CreateFontAtlas( const FontCreateInfo& info, Font& fontAsset, RawImage2D& atlasImg, const Config& config )
{
    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if ( !ft )
    {
        LOG_ERR( "Failed to initialize freetype" );
        return false;
    }
    msdfgen::FontHandle* font = msdfgen::loadFont( ft, GetAbsPath_FontFilename( info.filename ).c_str() );
    if ( !font )
    {
        LOG_ERR( "Failed to load font %s", GetAbsPath_FontFilename( info.filename ).c_str() );
        return false;
    }

    fontAsset.metrics.fontSize               = (float)info.glyphSize;
    fontAsset.metrics.maxSignedDistanceRange = fontAsset.metrics.fontSize * info.maxSignedDistance;

    const int MAX_GLYPH_SIZE = 2 * info.glyphSize;
    msdfgen::Range msdfRange( info.maxSignedDistance );

    std::vector<GlyphData> glyphDatas;
    glyphDatas.reserve( FONT_TOTAL_CHARACTERS );
    for ( u32 character = FONT_FIRST_CHARACTER_CODE; character <= FONT_LAST_CHARACTER_CODE; ++character )
    {
        GlyphData& data = glyphDatas.emplace_back();
        data.character  = (char)character;
        if ( IsWhiteSpace( data.character ) )
            continue;

        if ( !msdfgen::loadGlyph( data.shape, font, character, msdfgen::FONT_SCALING_EM_NORMALIZED ) )
        {
            LOG_ERR( "Failed to load glyph data for character '%u'", character );
            return false;
        }

        data.shape.normalize();
        const double maxCornerAngle = 3.0;
        msdfgen::edgeColoringInkTrap( data.shape, maxCornerAngle );
        msdfgen::Shape::Bounds bounds = data.shape.getBounds();
        double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
        l += msdfRange.lower, b += msdfRange.lower;
        r -= msdfRange.lower, t -= msdfRange.lower;

        float scale = (float)config.glyphSize;
        data.scale  = scale;
        if ( config.pxAlignOriginX )
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

        if ( config.pxAlignOriginY )
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

        if ( data.atlasSize.x >= MAX_GLYPH_SIZE || data.atlasSize.y >= MAX_GLYPH_SIZE )
        {
            LOG_ERR( "Glyph is too large for rendering into bitmap! Size %d x %d", data.atlasSize.x, data.atlasSize.y );
            return false;
        }
    }

    int width, height;
    if ( !PackAtlas( glyphDatas, width, height ) )
    {
        LOG_ERR( "Failed to pack all glyphs into an atlas!" );
        return false;
    }

    atlasImg = RawImage2D( width, height, ImageFormat::R8_G8_B8_A8_UNORM );
    atlasImg.Clear( vec4( 0, 0, 0, 1 ) );

#if USING( MTSDF )
    msdfgen::MSDFGeneratorConfig preConfig  = config.gconfig;
    msdfgen::MSDFGeneratorConfig postConfig = config.gconfig;
    if ( config.scanlinePass )
    {
        preConfig.errorCorrection.mode               = msdfgen::ErrorCorrectionConfig::DISABLED;
        preConfig.errorCorrection.distanceCheckMode  = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
        postConfig.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
    }
    msdfgen::Bitmap<float, 4> outBitmap( MAX_GLYPH_SIZE, MAX_GLYPH_SIZE );
#else  // #if USING( MTSDF )
    msdfgen::Bitmap<float, 1> outBitmap( MAX_GLYPH_SIZE, MAX_GLYPH_SIZE );
#endif // #else // #if USING( MTSDF )

    for ( const GlyphData& glyph : glyphDatas )
    {
        if ( IsWhiteSpace( glyph.character ) )
            continue;

        msdfgen::Vector2 scale     = msdfgen::Vector2( glyph.scale );
        msdfgen::Vector2 translate = msdfgen::Vector2( glyph.translation.x, glyph.translation.y );
        msdfgen::Projection proj( scale, translate );

#if USING( MTSDF )
        // glyph-generators.cpp::mtsdfGenerator()
        msdfgen::generateMTSDF( outBitmap, glyph.shape, proj, msdfRange, preConfig );
        if ( config.scanlinePass )
        {
            msdfgen::distanceSignCorrection( outBitmap, glyph.shape, proj, msdfgen::FILL_NONZERO );
            if ( postConfig.errorCorrection.mode != msdfgen::ErrorCorrectionConfig::DISABLED )
            {
                msdfgen::msdfErrorCorrection( outBitmap, glyph.shape, proj, msdfRange, postConfig );
            }
        }
#else  // #if USING( MTSDF )
        msdfgen::generateSDF( outBitmap, glyph.shape, proj, msdfRange, config.gconfig );
        if ( config.scanlinePass )
            msdfgen::distanceSignCorrection( outBitmap, glyph.shape, proj, msdfgen::FILL_NONZERO );
#endif // #else // #if USING( MTSDF )

        for ( int r = 0; r < glyph.atlasSize.y; ++r )
        {
            for ( int c = 0; c < glyph.atlasSize.x; ++c )
            {
                atlasImg.SetPixelFromFloat4( glyph.atlasPos.y + r, glyph.atlasPos.x + c, vec4( 0, 0, 0, 1 ) );
                for ( int i = 0; i < ( USING( MTSDF ) ? 4 : 1 ); ++i )
                {
                    float x = outBitmap( c, glyph.atlasSize.y - r - 1 )[i];
                    atlasImg.SetPixelFromFloat( glyph.atlasPos.y + r, glyph.atlasPos.x + c, i, Saturate( x ) );
                }
            }
        }
    }

#define DEBUG_ATLAS NOT_IN_USE

#if USING( DEBUG_ATLAS )
    atlasImg.Save( PG_ROOT_DIR "fontAtlas.png" );
#endif // #if USING( DEBUG_ATLAS )

    msdfgen::deinitializeFreetype( ft );

    if ( !GetFontMetrics( info.filename, glyphDatas, atlasImg, fontAsset ) )
    {
        LOG_ERR( "Could not get font metrics!" );
        return false;
    }

    return true;
}

bool FontConverter::ConvertInternal( ConstDerivedInfoPtr& info )
{
    Font font;
    font.SetName( info->name );

    Config config    = {};
    config.glyphSize = info->glyphSize;
    config.emRange   = info->maxSignedDistance;

    RawImage2D atlasImg;
    if ( !CreateFontAtlas( *info, font, atlasImg, config ) )
    {
        LOG_ERR( "Could not create font atlas for font '%s'", info->name.c_str() );
        return false;
    }

    GfxImage gfxImage;
    RawImage2DMipsToGfxImage( font.fontAtlasTexture, { atlasImg }, PixelFormat::R8_G8_B8_A8_UNORM );
    font.fontAtlasTexture.SetName( "font_atlas_" + info->name );

    std::string cacheName = GetCacheName( info );
    if ( !AssetCache::CacheAsset( assetType, cacheName, &font ) )
    {
        LOG_ERR( "Failed to cache asset %s %s (%s)", g_assetNames[assetType], info->name.c_str(), cacheName.c_str() );
        return false;
    }

    return true;
}

} // namespace PG
