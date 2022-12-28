#pragma once

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

// Note: BC6H_U16F works fine, but something seems wrong with Compressenator's BC6H_S16F
RawImage2D CompressToBC( const RawImage2D& image, const BCCompressorSettings& settings );
std::vector<RawImage2D> CompressToBC( const std::vector<RawImage2D>& images, const BCCompressorSettings& settings );
RawImage2D DecompressBC( const RawImage2D& compressedImage );
