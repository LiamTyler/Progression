#include "pt_image.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/gfx_image.hpp"
#include "shared/logger.hpp"

using namespace PG;

namespace PT
{

std::vector<ImageU8> g_textures = {};
std::unordered_map<std::string, TextureHandle> s_textureNameToHandleMap;


TextureHandle LoadTextureFromGfxImage( GfxImage* image )
{
    if ( g_textures.empty() )
    {
        g_textures.emplace_back();
    }
    PG_ASSERT( image && image->name.length() > 0 );
    auto it = s_textureNameToHandleMap.find( image->name );
    if ( it != s_textureNameToHandleMap.end() )
    {
        return it->second;
    }

    int numChannels = NumChannelsInPixelFromat( image->pixelFormat );
    PG_ASSERT( NumBytesPerChannel( image->pixelFormat ) == 1 );
    ImageU8 newImg( image->width, image->height );
    for ( int r = 0; r < (int)image->height; ++r )
    {
        for ( int c = 0; c < (int)image->width; ++c )
        {
            glm::u8vec4 newP( 0, 0, 0, 255 );
            for ( int i = 0; i < numChannels; ++i )
            {
                 newP[i] = image->pixels[numChannels * (r*image->width + c) + i];
            }
            newImg.SetPixel( r, c, newP );
        }
    }
    g_textures.emplace_back( std::move( newImg ) );
    TextureHandle handle = static_cast<TextureHandle>( g_textures.size() - 1 );
    s_textureNameToHandleMap[image->name] = handle;
    return handle;
}


ImageU8* GetTex( TextureHandle handle )
{
    return &g_textures[handle];
}

} // namespace PT