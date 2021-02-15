#include "gfx_image_converter.hpp"
#include "asset/image.hpp"
#include "asset/types/gfx_image.hpp"


namespace PG
{

static std::unordered_map< std::string, GfxImageSemantic > s_imageSemanticMap =
{
    { "DIFFUSE",    GfxImageSemantic::DIFFUSE },
    { "NORMAL",     GfxImageSemantic::NORMAL },
    { "METALNESS",  GfxImageSemantic::METALNESS },
    { "ROUGHNESS",  GfxImageSemantic::ROUGHNESS },
};


static std::string ImageSemanticToString( GfxImageSemantic semantic )
{
    const char* names[] =
    {
        "DIFFUSE",
        "NORMAL",
        "METALNESS",
        "ROUGHNESS",
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


void GfxImageConverter::Parse( const rapidjson::Value& value )
{
    static JSONFunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "name",           []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.name = v.GetString(); } },
        { "filename",       []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.filename = PG_ASSET_DIR + std::string( v.GetString() ); } },
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

    GfxImageCreateInfo* info = new GfxImageCreateInfo;
    info->semantic = GfxImageSemantic::NUM_IMAGE_SEMANTICS;
    info->imageType = Gfx::ImageType::TYPE_2D;
    mapping.ForEachMember( value, *info );

    if ( !PathExists( info->filename ) )
    {
        LOG_ERR( "Filename '%s' not found for GfxImage '%s', skipping image", info->filename.c_str(), info->name.c_str() );
        g_converterStatus.parsingError = true;
    }
    if ( info->semantic == GfxImageSemantic::NUM_IMAGE_SEMANTICS )
    {
        LOG_ERR( "Must specify a valid image semantic for image '%s'", info->name.c_str() );
        g_converterStatus.parsingError = true;
    }
    if ( info->imageType == Gfx::ImageType::NUM_IMAGE_TYPES )
    {
        LOG_ERR( "Must specify a valid imageType for image '%s'", info->name.c_str() );
        g_converterStatus.parsingError = true;
    }
    if ( info->dstPixelFormat == PixelFormat::INVALID )
    {
        switch ( info->semantic )
        {
        case GfxImageSemantic::DIFFUSE:
            info->dstPixelFormat = PixelFormat::R8_G8_B8_A8_SRGB;
            break;
        case GfxImageSemantic::NORMAL:
            info->dstPixelFormat = PixelFormat::R8_G8_B8_A8_UNORM;
            break;
        case GfxImageSemantic::METALNESS:
        case GfxImageSemantic::ROUGHNESS:
            info->dstPixelFormat = PixelFormat::R8_UNORM;
            break;
        default:
            LOG_ERR( "Semantic (%d) unknown when deciding final image format", info->semantic );
            break;
        }
    }

    m_parsedAssets.push_back( info );
}


std::string GfxImageConverter::GetFastFileName( const BaseAssetCreateInfo* baseInfo ) const
{
    static_assert( sizeof( GfxImageCreateInfo ) == 16 + 2 * sizeof( std::string ), "Dont forget to add new hash value" );

    GfxImageCreateInfo* info = (GfxImageCreateInfo*)baseInfo;
    std::string baseName = info->name;
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( static_cast< int >( info->semantic ) );
    baseName += "_" + std::to_string( static_cast< int >( info->imageType ) );
    baseName += "_" + std::to_string( static_cast< int >( info->dstPixelFormat ) );
    baseName += "_" + std::to_string( static_cast< int >( info->flipVertically ) );
    baseName += "_" + std::to_string( std::hash< std::string >{}( info->filename ) );
    FilenameSlashesToUnderscores( baseName );

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
    GfxImageCreateInfo* info = (GfxImageCreateInfo*)baseInfo;
    return IsFileOutOfDate( ffName, info->filename );
}


bool GfxImageConverter::ConvertSingle( const BaseAssetCreateInfo* baseInfo ) const
{
    GfxImageCreateInfo* info = (GfxImageCreateInfo*)baseInfo;
    LOG( "Converting image '%s'...", info->name.c_str() );
    GfxImage image;
    if ( !GfxImage_Load( &image, *info ) )
    {
        return false;
    }
    std::string fastfileName = GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    if ( !Fastfile_GfxImage_Save( &image, &serializer ) )
    {
        LOG_ERR( "Error while writing image '%s' to fastfile", image.name.c_str() );
        serializer.Close();
        DeleteFile( fastfileName );
        return false;
    }
    serializer.Close();

    return true;
}

} // namespace PG