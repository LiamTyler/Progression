#pragma once

#include "glm/glm.hpp"
#include <string>

namespace Progression
{

enum ImageFlagBits
{
    IMAGE_FLIP_VERTICALLY        = 1 << 0,
};
typedef uint32_t ImageFlags;

struct ImageCreateInfo
{
    std::string name;
    std::string filename;
    ImageFlags flags       = 0;
};


class Image
{
public:
    Image() = default;
    Image( int width, int height );
    ~Image();
    Image( Image&& src );
    Image& operator=( Image&& src );

    bool Load( ImageCreateInfo* createInfo );
    bool Save( const std::string& filename ) const;
    glm::vec4 GetPixel( int row, int col ) const;
    void SetPixel( int row, int col, const glm::vec4 &pixel );

    std::string name;
    int width              = 0;
    int height             = 0;
    glm::vec4* pixels      = nullptr;
    ImageFlags flags       = 0;
};

} // namespace Progression
