#pragma once

#include "math.hpp"
#include "resource/resource.hpp"

namespace PT
{

struct TextureCreateInfo
{
    std::string name;
    std::string filename;
    bool flipVertically = true;
};

struct Texture : public Resource
{
    Texture() = default;
    ~Texture();

    bool Load( const TextureCreateInfo& info );
    
    int GetWidth() const;
    int GetHeight() const;
    unsigned char* GetPixels() const;
    glm::vec4 GetPixel( float u, float v ) const;

private:
    int m_width             = 0;
    int m_height            = 0;
    unsigned char* m_pixels = nullptr;
};

} // namespace PT