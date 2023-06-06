#include "pt_image.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "core/image_processing.hpp"
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

    GfxImage decompressedImg;
    if ( PixelFormatIsCompressed( image->pixelFormat ) )
    {
        decompressedImg = DecompressGfxImage( *image );
    }

    image = &decompressedImg;
    std::shared_ptr<Texture> tex;
    if ( image->imageType == Gfx::ImageType::TYPE_2D )
    {
        tex = std::make_shared<Texture2D>( image->width, image->height, image->mipLevels, image->pixelFormat, (void*)image->pixels );
        //RawImage2D img( image->width, image->height, PixelFormatToImageFormat( image->pixelFormat ), std::static_pointer_cast<Texture2D>( tex )->Raw( 0 ) );
        //img.Save( PG_ROOT_DIR + image->name + (PixelFormatIsFloat( image->pixelFormat ) ? ".exr" : ".png") );
    }
    else if ( image->imageType == Gfx::ImageType::TYPE_CUBEMAP )
    {
        void* faceData[6];
        for ( int face = 0; face < 6; ++face )
        {
            faceData[face] = image->GetPixels( face, 0 );
        }
        tex = std::make_shared<TextureCubeMap>( image->width, image->height, image->mipLevels, image->pixelFormat, faceData );
    }
    else
    {
        LOG_ERR( "Non 2d or cubemap images are not supported" );
        return TEXTURE_HANDLE_INVALID;
    }

    g_textures.push_back( tex );
    TextureHandle handle = static_cast<TextureHandle>( g_textures.size() - 1 );
    s_textureNameToHandleMap[image->name] = handle;

    if ( image == &decompressedImg )
    {
        image->Free();
    }

    return handle;
}


Texture* GetTex( TextureHandle handle )
{
    return g_textures[handle].get();
}

} // namespace PT