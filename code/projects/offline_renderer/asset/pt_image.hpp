#pragma once

#include "image.hpp"
#include <vector>

namespace PG
{
    struct GfxImage;
} // namespace PG

namespace PT
{

using TextureHandle = uint16_t;
constexpr TextureHandle TEXTURE_HANDLE_INVALID = 0;

TextureHandle LoadTextureFromGfxImage( PG::GfxImage* image );

ImageU8* GetTex( TextureHandle handle );

} // namespace PT