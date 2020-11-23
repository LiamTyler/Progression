#pragma once

#include "core/pixel_formats.hpp"

namespace PG
{

uint32_t CalculateNumMips( uint32_t width, uint32_t height );

// if numMuips is unspecified (0), assume all mips are in use
size_t CalculateTotalFaceSizeWithMips( uint32_t width, uint32_t height, PixelFormat format, uint32_t numMips = 0 );

size_t CalculateTotalImageBytes( PixelFormat format, uint32_t width, uint32_t height, uint32_t depth = 1, uint32_t arrayLayers = 1, uint32_t mipLevels = 1 );

} // namespace PG
