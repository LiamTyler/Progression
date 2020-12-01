#include "asset/image.hpp"
#include "core/assert.hpp"
#include "asset/types/gfx_image.hpp"
#include "asset/asset_versions.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

using namespace PG;

extern void AddFastfileDependency( const std::string& file );

std::vector< GfxImageCreateInfo > g_parsedImages;
std::vector< GfxImageCreateInfo > g_outOfDateImages;
extern bool g_parsingError;

static std::unordered_map< std::string, GfxImageSemantic > imageSemanticMap =
{
    { "DIFFUSE", GfxImageSemantic::DIFFUSE },
    { "NORMAL",  GfxImageSemantic::NORMAL }
};


static std::string ImageSemanticToString( GfxImageSemantic semantic )
{
    const char* names[] =
    {
        "DIFFUSE",
        "NORMAL",
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


void GfxImage_Parse( const rapidjson::Value& value )
{
    // static FunctionMapper< void, std::vector< std::string >& > cubemapParser(
    // {
    //     { "right",  []( rapidjson::Value& v, std::vector< std::string >& files ) { files[0] = v.GetString(); } },
    //     { "left",   []( rapidjson::Value& v, std::vector< std::string >& files ) { files[1] = v.GetString(); } },
    //     { "top",    []( rapidjson::Value& v, std::vector< std::string >& files ) { files[3] = v.GetString(); } },
    //     { "bottom", []( rapidjson::Value& v, std::vector< std::string >& files ) { files[2] = v.GetString(); } },
    //     { "back",   []( rapidjson::Value& v, std::vector< std::string >& files ) { files[4] = v.GetString(); } },
    //     { "front",  []( rapidjson::Value& v, std::vector< std::string >& files ) { files[5] = v.GetString(); } },
    // });

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
                auto it = imageSemanticMap.find( semanticName );
                if ( it == imageSemanticMap.end() )
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

    GfxImageCreateInfo info;
    info.semantic = GfxImageSemantic::NUM_IMAGE_SEMANTICS;
    info.imageType = Gfx::ImageType::TYPE_2D;
    mapping.ForEachMember( value, info );

    if ( !PathExists( info.filename ) )
    {
        LOG_ERR( "Filename '%s' not found for GfxImage '%s', skipping image", info.filename.c_str(), info.name.c_str() );
        g_parsingError = true;
    }
    if ( info.semantic == GfxImageSemantic::NUM_IMAGE_SEMANTICS )
    {
        LOG_ERR( "Must specify a valid image semantic for image '%s'", info.name.c_str() );
        g_parsingError = true;
    }
    if ( info.imageType == Gfx::ImageType::NUM_IMAGE_TYPES )
    {
        LOG_ERR( "Must specify a valid imageType for image '%s'", info.name.c_str() );
        g_parsingError = true;
    }
    if ( info.dstPixelFormat == PixelFormat::INVALID )
    {
        switch ( info.semantic )
        {
        case GfxImageSemantic::DIFFUSE:
            info.dstPixelFormat = PixelFormat::R8_G8_B8_A8_SRGB;
            break;
        case GfxImageSemantic::NORMAL:
            info.dstPixelFormat = PixelFormat::R8_G8_B8_A8_UNORM;
            break;
        default:
            break;
        }
    }

    g_parsedImages.push_back( info );
}


static std::string GfxImage_GetFastFileName( const GfxImageCreateInfo& info )
{
    static_assert( sizeof( GfxImageCreateInfo ) == 16 + 2 * sizeof( std::string ), "Dont forget to add new hash value" );

    std::string baseName = info.name;
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( static_cast< int >( info.semantic ) );
    baseName += "_" + std::to_string( static_cast< int >( info.imageType ) );
    baseName += "_" + std::to_string( static_cast< int >( info.dstPixelFormat ) );
    baseName += "_" + std::to_string( static_cast< int >( info.flipVertically ) );
    baseName += "_" + std::to_string( std::hash< std::string >{}( info.filename ) );

    std::string fullName = PG_ASSET_DIR "cache/images/" + baseName + ".ffi";
    return fullName;
}


static bool GfxImage_IsOutOfDate( const GfxImageCreateInfo& info )
{
    std::string ffName = GfxImage_GetFastFileName( info );
    AddFastfileDependency( ffName );
    return IsFileOutOfDate( ffName, info.filename );
}


static bool GfxImage_ConvertSingle( const GfxImageCreateInfo& info )
{
    LOG( "Converting image '%s'...", info.name.c_str() );
    GfxImage image;
    if ( !GfxImage_Load( &image, info ) )
    {
        return false;
    }
    std::string fastfileName = GfxImage_GetFastFileName( info );
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


int GfxImage_CheckDependencies()
{
    int outOfDate = 0;
    for ( size_t i = 0; i < g_parsedImages.size(); ++i )
    {
        if ( GfxImage_IsOutOfDate( g_parsedImages[i] ) )
        {
            g_outOfDateImages.push_back( g_parsedImages[i] );
            ++outOfDate;
        }
    }

    return outOfDate;
}


int GfxImage_Convert()
{
    if ( g_outOfDateImages.size() == 0 )
    {
        return 0;
    }

    int couldNotConvert = 0;
    for ( int i = 0; i < (int)g_outOfDateImages.size(); ++i )
    {
        if ( !GfxImage_ConvertSingle( g_outOfDateImages[i] ) )
        {
            ++couldNotConvert;
        }
    }

    return couldNotConvert;
}


bool GfxImage_BuildFastFile( Serializer* serializer )
{
    for ( size_t i = 0; i < g_parsedImages.size(); ++i )
    {
        std::string ffiName = GfxImage_GetFastFileName( g_parsedImages[i] );
        MemoryMapped inFile;
        if ( !inFile.open( ffiName ) )
        {
            LOG_ERR( "Could not open file '%s'", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_GFX_IMAGE );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}