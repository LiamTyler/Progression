#pragma once

#include <string>
#include <vector>
#include <memory>

namespace PG
{

enum class CompositeType : uint8_t
{
    REMAP,

    COUNT
};

struct OutputChannelInfo
{
    int outputChannelIndex = -1;
    int inputImageIndex = -1;
    int inputChannelIndex = -1;
    float scale = 1.0f;
    float bias = 0.0f;
};

struct CompositeImageInput
{
    CompositeType compositeType;
    bool saveAsSRGBFormat;
    std::string sourceImages[4];
    std::vector<OutputChannelInfo> outputChannelInfos;
};

struct FloatImage
{
    uint32_t width;
    uint32_t height;
    uint32_t numChannels;
    std::shared_ptr<float[]> pixels;

    FloatImage() : FloatImage( 0, 0, 0 ) {}
    FloatImage( uint32_t inWidth, uint32_t inHeight, uint32_t inChannels ) : width( inWidth ), height( inHeight ), numChannels( inChannels )
    {
        pixels = nullptr;
        size_t count = width * height * numChannels;
        pixels = count ? std::make_shared<float[]>( count ) : nullptr;
    }

    bool Load( const std::string& filename );

    size_t TotalPixels() const { return width * height; }
    size_t TotalBytes() const { return width * height; }
};

FloatImage CompositeImage( const CompositeImageInput& input );

} // namespace PG
