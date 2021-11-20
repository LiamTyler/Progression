#pragma once

#include "math.hpp"
#include "resource/resource.hpp"

namespace PT
{

struct SkyboxCreateInfo
{
    std::string name;
    std::string right;
    std::string left;
    std::string top;
    std::string bottom;
    std::string back;
    std::string front;
    bool flipVertically = true;
};

struct Skybox : public Resource
{
    Skybox() = default;
    ~Skybox();

    bool Load( const SkyboxCreateInfo& info );
    
    int GetWidth() const;
    int GetHeight() const;
    glm::vec4* GetPixels() const;
    glm::vec4 GetPixel( int face, int r, int c ) const;
    glm::vec4 GetPixel( const Ray& ray ) const;

private:
    int m_width         = 0;
    int m_height        = 0;
    glm::vec4* m_pixels = nullptr;
};

} // namespace PT