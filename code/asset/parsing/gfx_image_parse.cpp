#include "gfx_image_parse.hpp"
#include "image.hpp"

namespace PG
{

static std::unordered_map< std::string, ImageInputType > s_inputTypeMap =
{
    { "REGULAR_2D",        ImageInputType::REGULAR_2D },
    { "EQUIRECTANGULAR",   ImageInputType::EQUIRECTANGULAR },
    { "FLATTENED_CUBEMAP", ImageInputType::FLATTENED_CUBEMAP },
    { "INDIVIDUAL_FACES",  ImageInputType::INDIVIDUAL_FACES },
};

static std::unordered_map< std::string, GfxImageSemantic > s_imageSemanticMap =
{
    { "DIFFUSE",         GfxImageSemantic::DIFFUSE },
    { "NORMAL",          GfxImageSemantic::NORMAL },
    { "METALNESS",       GfxImageSemantic::METALNESS },
    { "ROUGHNESS",       GfxImageSemantic::ROUGHNESS },
    { "ENVIRONMENT_MAP", GfxImageSemantic::ENVIRONMENT_MAP },
};


static std::unordered_map< std::string, Gfx::ImageType > imageTypeMap =
{
    { "TYPE_1D",            Gfx::ImageType::TYPE_1D },
    { "TYPE_1D_ARRAY",      Gfx::ImageType::TYPE_1D_ARRAY },
    { "TYPE_2D",            Gfx::ImageType::TYPE_2D },
    { "TYPE_2D_ARRAY",      Gfx::ImageType::TYPE_2D_ARRAY },
    { "TYPE_CUBEMAP",       Gfx::ImageType::TYPE_CUBEMAP },
    { "TYPE_CUBEMAP_ARRAY", Gfx::ImageType::TYPE_CUBEMAP_ARRAY },
    { "TYPE_3D",            Gfx::ImageType::TYPE_3D },
};


bool GfxImageParser::ParseInternal( const rapidjson::Value& value, InfoPtr info )
{
    #define ABS_PATH( v ) PG_ASSET_DIR + std::string( v.GetString() )
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "filename", []( const rapidjson::Value& v, GfxImageCreateInfo& s )
            {
                s.inputType = ImageInputType::REGULAR_2D;
                s.filename = ABS_PATH( v );
            }
        },
        { "flattenedCubemapFilename", []( const rapidjson::Value& v, GfxImageCreateInfo& s )
            {
                s.inputType = ImageInputType::FLATTENED_CUBEMAP;
                s.filename = ABS_PATH( v );
            }
        },
        { "equirectangularFilename",  []( const rapidjson::Value& v, GfxImageCreateInfo& s )
            {
                s.inputType = ImageInputType::EQUIRECTANGULAR;
                s.filename = ABS_PATH( v );
            }
        },
        { "left",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[CUBEMAP_FACE_LEFT]   = ABS_PATH( v ); } },
        { "right",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[CUBEMAP_FACE_RIGHT]  = ABS_PATH( v ); } },
        { "front",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[CUBEMAP_FACE_FRONT]  = ABS_PATH( v ); } },
        { "back",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[CUBEMAP_FACE_BACK]   = ABS_PATH( v ); } },
        { "top",    []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[CUBEMAP_FACE_TOP]    = ABS_PATH( v ); } },
        { "bottom", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[CUBEMAP_FACE_BOTTOM] = ABS_PATH( v ); } },
        { "flipVertically", []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.flipVertically = v.GetBool(); } },
        { "imageType",      []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                std::string imageName = v.GetString();
                auto it = imageTypeMap.find( imageName );
                if ( it == imageTypeMap.end() )
                {
                    LOG_ERR( "No GfxImageType found matching '%s'", imageName.c_str() );
                    i.imageType = Gfx::ImageType::NUM_IMAGE_TYPES;
                }
                else
                {
                    i.imageType = it->second;
                }
            }
        },
        { "semantic",  []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                std::string semanticName = v.GetString();
                auto it = s_imageSemanticMap.find( semanticName );
                if ( it == s_imageSemanticMap.end() )
                {
                    LOG_ERR( "No image semantic found matching '%s'", semanticName.c_str() );
                    i.semantic = GfxImageSemantic::NUM_IMAGE_SEMANTICS;
                }
                else
                {
                    i.semantic = it->second;
                }
            }
        },
        { "dstFormat", []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                i.dstPixelFormat = PixelFormatFromString( v.GetString() );
                if ( i.dstPixelFormat == PixelFormat::INVALID )
                {
                    LOG_ERR( "Invalid dstFormat '%s'", v.GetString() );
                }
            }
        },
    });
    mapping.ForEachMember( value, *info );

    if ( info->semantic == GfxImageSemantic::NUM_IMAGE_SEMANTICS ) PARSE_ERROR( "Must specify a valid image semantic for image '%s'", info->name.c_str() );
    if ( info->semantic == GfxImageSemantic::ENVIRONMENT_MAP )
    {
        info->imageType = Gfx::ImageType::TYPE_CUBEMAP;
    }

    int numFaces = 0;
    for ( int i = 0; i < 6; ++i )
    {
        if ( !info->faceFilenames[i].empty() )
        {
            info->inputType = ImageInputType::INDIVIDUAL_FACES;
            ++numFaces;
        }
    }
    if ( info->inputType == ImageInputType::NUM_IMAGE_INPUT_TYPES ) PARSE_ERROR( "Could not deduce ImageInputType. No filename given, nor 6 cubemap faces for image '%s'", info->name.c_str() );
    if ( info->inputType == ImageInputType::INDIVIDUAL_FACES && numFaces != 6 ) PARSE_ERROR( "Please specify all 6 cubemap faces for image '%s'", info->name.c_str() );

    return true;
}

} // namespace PG