#include "image.hpp"
#include "assert.hpp"
#include "gfx_image.hpp"
#include "asset_versions.hpp"
#include "utils/filesystem.hpp"
#include "utils/file_dependency.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/serializer.hpp"

using namespace Progression;

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


static std::unordered_map< std::string, GfxImageType > imageTypeMap =
{
    { "TYPE_1D",            GfxImageType::TYPE_1D },
    { "TYPE_1D_ARRAY",      GfxImageType::TYPE_1D_ARRAY },
    { "TYPE_2D",            GfxImageType::TYPE_2D },
    { "TYPE_2D_ARRAY",      GfxImageType::TYPE_2D_ARRAY },
    { "TYPE_CUBEMAP",       GfxImageType::TYPE_CUBEMAP },
    { "TYPE_CUBEMAP_ARRAY", GfxImageType::TYPE_CUBEMAP_ARRAY },
    { "TYPE_3D",            GfxImageType::TYPE_3D },
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

    static FunctionMapper< GfxImageCreateInfo& > mapping(
    {
        { "name",      []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.name = v.GetString(); } },
        { "filename",  []( const rapidjson::Value& v, GfxImageCreateInfo& i ) { i.filename = PG_ASSET_DIR "images/" + std::string( v.GetString() ); } },
        { "imageType", []( const rapidjson::Value& v, GfxImageCreateInfo& i )
            {
                std::string imageName = v.GetString();
                auto it = imageTypeMap.find( imageName );
                if ( it == imageTypeMap.end() )
                {
                    LOG_WARN( "No GfxImageType found matching '%s'\n", imageName.c_str() );
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
                    LOG_WARN( "No image semantic found matching '%s', assuming TYPE_2D\n", semanticName.c_str() );
                    i.imageType = GfxImageType::TYPE_2D;
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
                    LOG_ERR( "Invalid dstFormat '%s'\n", v.GetString() );
                }
            }
        },
    });

    GfxImageCreateInfo info;
    mapping.ForEachMember( value, info );

    if ( !FileExists( info.filename ) )
    {
        LOG_ERR( "Filename '%s' not found for GfxImage '%s', skipping image\n", info.filename.c_str(), info.name.c_str() );
        g_parsingError = true;
    }
    if ( info.dstPixelFormat == PixelFormat::INVALID )
    {
        LOG_ERR( "Must specify a valid pixel dstFormat for image '%s'\n", info.name.c_str() );
        g_parsingError = true;
    }

    g_parsedImages.push_back( info );
}


static std::string GfxImage_GetFastFileName( const GfxImageCreateInfo& info )
{
    static_assert( sizeof( GfxImageCreateInfo ) == 16 + 2 * sizeof( std::string ), "Dont forget to add new hash value" );

    std::string baseName = GetFilenameStem( info.name );
    baseName += "_v" + std::to_string( PG_GFX_IMAGE_VERSION );
    baseName += "_" + std::to_string( static_cast< int >( info.semantic ) );
    baseName += "_" + std::to_string( static_cast< int >( info.imageType ) );
    baseName += "_" + std::to_string( static_cast< int >( info.dstPixelFormat ) );
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
    LOG( "Converting image '%s'...\n", info.name.c_str() );
    std::string fastfileName = GfxImage_GetFastFileName( info );
    Serializer serializer;
    if ( !serializer.OpenForWrite( fastfileName ) )
    {
        return false;
    }
    GfxImage image;
    if ( !GfxImage_Load( &image, info ) )
    {
        return false;
    }
    if ( !Fastfile_GfxImage_Save( &image, &serializer ) )
    {
        LOG_ERR( "Error while writing image '%s' to fastfile\n", image.name.c_str() );
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
            LOG_ERR( "Could not open file '%s'\n", ffiName.c_str() );
            return false;
        }
        
        serializer->Write( AssetType::ASSET_TYPE_GFX_IMAGE );
        serializer->Write( inFile.getData(), inFile.size() );
        inFile.close();
    }

    return true;
}