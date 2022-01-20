#include "pt_image.hpp"
#include "asset/asset_file_database.hpp"
#include "asset/types/gfx_image.hpp"
#include "shared/logger.hpp"

using namespace PG;

namespace PT
{

std::vector<ImageU8> g_textures = {};
std::unordered_map<std::string, TextureHandle> s_textureNameToHandleMap;


TextureHandle LoadTextureFromGfxImage( GfxImage* image )
{
    PG_ASSERT( image && image->name.length() > 0 );
    auto it = s_textureNameToHandleMap.find( image->name );
    if ( it != s_textureNameToHandleMap.end() )
    {
        return it->second;
    }
    return 0;

    //auto createInfo = AssetDatabase::FindAssetInfo< GfxImageCreateInfo >( ASSET_TYPE_GFX_IMAGE, image->name );
    //if ( !createInfo )
    //{
    //    s_textureNameToHandleMap[image->name] = TEXTURE_HANDLE_INVALID;
    //    LOG_ERR( "No asset create info found for image '%s'", image->name.c_str() );
    //    return TEXTURE_HANDLE_INVALID;
    //}
    //Image2DCreateInfo srcImgCreateInfo;
    //srcImgCreateInfo.filename = createInfo->filename;
    //srcImgCreateInfo.flipVertically = createInfo->flipVertically;
    //ImageU8 srcImg;
    //if ( !srcImg.Load( srcImgCreateInfo ) )
    //{
    //    s_textureNameToHandleMap[image->name] = TEXTURE_HANDLE_INVALID;
    //    return TEXTURE_HANDLE_INVALID;
    //}
    //
    //g_textures.push_back( {} );
    //TextureHandle handle = static_cast< TextureHandle >( g_textures.size() - 1 );
    //s_textureNameToHandleMap[image->name] = handle;
    //return handle;
}


ImageU8* GetTex( TextureHandle handle )
{
    return &g_textures[handle];
}

} // namespace PT