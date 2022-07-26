#pragma once

#include "image.hpp"
#include <vector>

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

FloatImage CompositeImage( const CompositeImageInput& input );

} // namespace PG
