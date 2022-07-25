#pragma once

#include <cstdint>
#include "image.hpp"

enum class CompressionQuality
{
    LOWEST,
    MEDIUM,
    HIGHEST,

    COUNT
};

struct BCCompressorSettings
{
    ImageFormat format = ImageFormat::INVALID;
    CompressionQuality quality = CompressionQuality::MEDIUM;
    int bc4SourceChannel  = 0;
    int bc5SourceChannel1 = 0;
    int bc5SourceChannel2 = 1;
};

RawImage2D CompressToBC( RawImage2D image, const BCCompressorSettings& settings );
