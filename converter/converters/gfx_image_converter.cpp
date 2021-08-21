#include "gfx_image_converter.hpp"
#include "asset/image.hpp"
#include "asset/types/gfx_image.hpp"
#include "utils/hash.hpp"


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


static std::string ImageSemanticToString( GfxImageSemantic semantic )
{
    const char* names[] =
    {
        "DIFFUSE",
        "NORMAL",
        "METALNESS",
        "ROUGHNESS",
        "ENVIRONMENT_MAP",
    };

    static_assert( ARRAY_COUNT( names ) == static_cast< int >( GfxImageSemantic::NUM_IMAGE_SEMANTICS ) );
    return names[static_cast< int >( semantic )];
}


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



std::shared_ptr<BaseAssetCreateInfo> GfxImageConverter::Parse( const rapidjson::Value& value, std::shared_ptr<const BaseAssetCreateInfo> parent )
{
    #define ABS_PATH( v ) PG_ASSET_DIR + std::string( v.GetString() )
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "filename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.inputType = ImageInputType::REGULAR_2D; s.filename = ABS_PATH( v ); } },
        { "flattenedCubemapFilename", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.inputType = ImageInputType::FLATTENED_CUBEMAP; s.filename = ABS_PATH( v ); } },
        { "equirectangularFilename",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.inputType = ImageInputType::EQUIRECTANGULAR; s.filename = ABS_PATH( v ); } },
        { "left",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[FACE_LEFT]   = ABS_PATH( v ); } },
        { "right",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[FACE_RIGHT]  = ABS_PATH( v ); } },
        { "front",  []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[FACE_FRONT]  = ABS_PATH( v ); } },
        { "back",   []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[FACE_BACK]   = ABS_PATH( v ); } },
        { "top",    []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[FACE_TOP]    = ABS_PATH( v ); } },
        { "bottom", []( const rapidjson::Value& v, GfxImageCreateInfo& s ) { s.faceFilenames[FACE_BOTTOM] = ABS_PATH( v ); } },
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

    auto info = std::make_shared<GfxImageCreateInfo>();
    if ( parent )
    {
        *info = *std::static_pointer_cast<const GfxImageCreateInfo>( parent );
    }
    mapping.ForEachMember( value, *info );

    if ( info->semantic == GfxImageSemantic::NUM_IMAGE_SEMANTICS ) PARSE_ERROR( "Must specify a valid image semantic for image '%s'", info->name.c_str() );
    if ( info->imageType == Gfx::ImageType::NUM_IMAGE_TYPES ) PARSE_ERROR( "Must specify a valid imageType for image '%s'", info->name.c_str() );

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

    return info;
}


std::string GfxImageConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    static_assert( sizeof( GfxImageCreateInfo ) == 24 + 8 * sizeof( std::string ), "Dont forget to add new hash value" );

    const GfxImageCreateInfo* info = (const GfxImageCreateInfo*)baseInfo;
    std::string baseName = info->name;
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( static_cast< int >( info->semantic ) );
    baseName += "_" + std::to_string( static_cast< int >( info->imageType ) );
    baseName += "_" + std::to_string( static_cast< int >( info->dstPixelFormat ) );
    baseName += "_" + std::to_string( static_cast< int >( info->flipVertically ) );
    size_t inputFileHash = 0;
    if ( info->inputType == ImageInputType::INDIVIDUAL_FACES )
    {
        for ( int i = 0; i < 6; ++i )
        {
            HashCombine( inputFileHash, info->faceFilenames[i] );
        }
    }
    else
    {
        inputFileHash = Hash( info->filename );
    }
    baseName += "_" + std::to_string( inputFileHash );

    std::string fullName = PG_ASSET_DIR "cache/images/" + baseName + ".ffi";
    return fullName;
}


bool GfxImageConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    std::string ffName = GetFastFileName( baseInfo );
    AddFastfileDependency( ffName );
    const GfxImageCreateInfo* info = (const GfxImageCreateInfo*)baseInfo;
    return IsFileOutOfDate( ffName, info->filename );
}


bool GfxImageConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    return ConvertSingleInternal< GfxImage, GfxImageCreateInfo >( baseInfo );
}

} // namespace PG

/* SAVE: for when cubemap loading is added for GfxImages
void EnvironmentMapConverter::Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< EnvironmentMapCreateInfo& > mapping(
    {
        { "name",           []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.name = v.GetString(); } },
        { "flipVertically", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.flipVertically = v.GetBool(); } },
        { "flattenedCubemapFilename", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.flattenedCubemapFilename = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "equirectangularFilename",  []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.equirectangularFilename  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "left",   []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_LEFT]   = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "right",  []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_RIGHT]  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "front",  []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_FRONT]  = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "back",   []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_BACK]   = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "top",    []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_TOP]    = PG_ASSET_DIR + std::string( v.GetString() ); } },
        { "bottom", []( const rapidjson::Value& v, EnvironmentMapCreateInfo& s ) { s.faceFilenames[FACE_BOTTOM] = PG_ASSET_DIR + std::string( v.GetString() ); } },
    });

    EnvironmentMapCreateInfo* info = new EnvironmentMapCreateInfo;
    mapping.ForEachMember( value, *info );
    m_parsedAssets.push_back( info );
}


std::string EnvironmentMapConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    EnvironmentMapCreateInfo* info = (EnvironmentMapCreateInfo*)baseInfo;
    std::string baseName = info->name;
    size_t fileHashes = 0;
    HashCombine( fileHashes, info->equirectangularFilename );
    HashCombine( fileHashes, info->flattenedCubemapFilename );
    for ( int i = 0; i < 6; ++i )
    {
        HashCombine( fileHashes, info->faceFilenames[i] );
    }
    baseName += "_" + std::to_string( fileHashes );
    baseName += "_v" + std::to_string( PG_ENVIRONMENT_MAP_VERSION );

    std::string fullName = PG_ASSET_DIR "cache/environment_maps/" + baseName + ".ffi";
    return fullName;
}


bool EnvironmentMapConverter::IsAssetOutOfDate( const BaseAssetCreateInfo* baseInfo )
{
    if ( g_converterConfigOptions.force )
    {
        return true;
    }

    EnvironmentMapCreateInfo* info = (EnvironmentMapCreateInfo*)baseInfo;
    std::string ffName = GetFastFileName( info );
    AddFastfileDependency( ffName );
    if ( !info->equirectangularFilename.empty() ) return IsFileOutOfDate( ffName, info->equirectangularFilename );
    else if ( !info->flattenedCubemapFilename.empty() ) return IsFileOutOfDate( ffName, info->flattenedCubemapFilename );
    else if ( !info->equirectangularFilename.empty() ) return IsFileOutOfDate( ffName, info->faceFilenames, 6 );
    else return true;
}
*/