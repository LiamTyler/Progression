#include "gfx_image_parse.hpp"
#include "image.hpp"

namespace PG
{

BEGIN_STR_TO_ENUM_MAP( GfxImageSemantic )
    STR_TO_ENUM_VALUE( GfxImageSemantic, COLOR )
    STR_TO_ENUM_VALUE( GfxImageSemantic, GRAY )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ALBEDO_METALNESS )
    STR_TO_ENUM_VALUE( GfxImageSemantic, ENVIRONMENT_MAP )
END_STR_TO_ENUM_MAP( GfxImageSemantic, NUM_IMAGE_SEMANTICS )

bool GfxImageParser::ParseInternal( const rapidjson::Value& value, DerivedInfoPtr info )
{
    #define ABS_PATH( v ) PG_ASSET_DIR + std::string( v.GetString() )
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "filename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[0] = ABS_PATH( v ); } },
        { "left",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_LEFT]   = ABS_PATH( v ); } },
        { "right",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_RIGHT]  = ABS_PATH( v ); } },
        { "front",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_FRONT]  = ABS_PATH( v ); } },
        { "back",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BACK]   = ABS_PATH( v ); } },
        { "top",    []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_TOP]    = ABS_PATH( v ); } },
        { "bottom", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.filenames[CUBEMAP_FACE_BOTTOM] = ABS_PATH( v ); } },
        { "flipVertically", []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.flipVertically = v.GetBool(); } },
        { "semantic",  []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { GfxImageSemantic_StringToEnum( v.GetString() ); } },
        { "dstFormat", []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                i.dstPixelFormat = PixelFormatFromString( v.GetString() );
                if ( i.dstPixelFormat == PixelFormat::INVALID )
                {
                    LOG_ERR( "Invalid dstFormat '%s'", v.GetString() );
                }
            }
        },
        { "clampHorizontal", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampHorizontal = v.GetBool(); } },
        { "clampVertical",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.clampVertical = v.GetBool(); } },
    });
    mapping.ForEachMember( value, *info );

    return true;
}

} // namespace PG