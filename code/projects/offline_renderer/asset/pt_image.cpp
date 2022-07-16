#include "pt_image.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "shared/logger.hpp"

using namespace PG;

namespace PT
{

std::vector<std::shared_ptr<Texture>> g_textures = { nullptr };
std::unordered_map<std::string, TextureHandle> s_textureNameToHandleMap;


TextureHandle LoadTextureFromGfxImage( GfxImage* image )
{
    PG_ASSERT( image && image->name.length() > 0 );
    auto it = s_textureNameToHandleMap.find( image->name );
    if ( it != s_textureNameToHandleMap.end() )
    {
        return it->second;
    }

    PG_ASSERT( !PixelFormatIsCompressed( image->pixelFormat ) );
    std::shared_ptr<Texture> tex;
    if ( image->imageType == Gfx::ImageType::TYPE_2D )
    {
        int numChannels = NumChannelsInPixelFromat( image->pixelFormat );
        PG_ASSERT( NumBytesPerChannel( image->pixelFormat ) == 1 );
        tex = std::make_shared<Texture2D<uint8_t,4,true,false,false>>( image->width, image->height, image->mipLevels, (void*)image->pixels );
    }
    else if ( image->imageType == Gfx::ImageType::TYPE_CUBEMAP )
    {
        int numChannels = NumChannelsInPixelFromat( image->pixelFormat );
        int bytesPerChannel = NumBytesPerChannel( image->pixelFormat );
        void* faceData[6];
        for ( int face = 0; face < 6; ++face )
        {
            faceData[face] = image->GetPixels( face, 0 );
        }
        
        switch ( numChannels )
        {
        case 3:
            switch ( bytesPerChannel )
            {
            case 1:
                tex = std::make_shared<TextureCubeMap<uint8_t,3>>( image->width, image->height, image->mipLevels, faceData );
                break;
            case 2:
                tex = std::make_shared<TextureCubeMap<float16,3>>( image->width, image->height, image->mipLevels, faceData );
                break;
            case 4:
                tex = std::make_shared<TextureCubeMap<float,3>>( image->width, image->height, image->mipLevels, faceData );
                break;
            }
            break;
        case 4:
            switch ( bytesPerChannel )
            {
            case 1:
                tex = std::make_shared<TextureCubeMap<uint8_t,4>>( image->width, image->height, image->mipLevels, faceData );
                break;
            case 2:
                tex = std::make_shared<TextureCubeMap<float16,4>>( image->width, image->height, image->mipLevels, faceData );
                break;
            case 4:
                tex = std::make_shared<TextureCubeMap<float,4>>( image->width, image->height, image->mipLevels, faceData );
                break;
            }
            break;
        default:
            PG_ASSERT( false, "only cubemaps of 3 or 4 channels supported" );
        }
        
    }
    else
    {
        LOG_ERR( "Non 2d or cubemap images are not supported" );
        return TEXTURE_HANDLE_INVALID;
    }

    g_textures.push_back( tex );
    TextureHandle handle = static_cast<TextureHandle>( g_textures.size() - 1 );
    s_textureNameToHandleMap[image->name] = handle;
    return handle;
}


Texture* GetTex( TextureHandle handle )
{
    return g_textures[handle].get();
}

} // namespace PT