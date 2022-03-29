#pragma once

#include "bc_compression.hpp"
#include "image.hpp"

bool Decompress_BC( CompressionFormat format, uint8_t* compressedData, int width, int height, ImageU8& decompressedImage );