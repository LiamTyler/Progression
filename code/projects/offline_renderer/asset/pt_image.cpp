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

    int numChannels = NumChannelsInPixelFromat( image->pixelFormat );
    PG_ASSERT( NumBytesPerChannel( image->pixelFormat ) == 1 );
    PG_ASSERT( !PixelFormatIsCompressed( image->pixelFormat ) );

    std::shared_ptr<Texture> tex = std::make_shared<Texture2D<uint8_t,4,true,false,false>>( image->width, image->height, (void*)image->pixels );

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