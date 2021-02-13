#pragma once

#include "glm/glm.hpp"
#include <string>

namespace PG
{

enum ImageFlagBits
{
    IMAGE_FLIP_VERTICALLY        = 1 << 0,
};
typedef uint32_t ImageFlags;

struct Image2DCreateInfo
{
    std::string name;
    std::string filename;
    ImageFlags flags       = 0;
};


class Image2D
{
public:
    Image2D() = default;
    Image2D( int width, int height );
    ~Image2D();
    Image2D( Image2D&& src );
    Image2D& operator=( Image2D&& src );

    bool Load( Image2DCreateInfo* createInfo );
    bool Save( const std::string& filename ) const;

    glm::vec4 GetPixel( int row, int col ) const;
    void SetPixel( int row, int col, const glm::vec4 &pixel );
    void SetPixel( int row, int col, const glm::vec3 &pixel );
    glm::vec4 SampleNearest( glm::vec2 uv ) const;
    glm::vec4 SampleBilinear( glm::vec2 uv ) const;
    glm::vec4 SampleEquirectangularNearest( const glm::vec3& dir ) const;
    glm::vec4 SampleEquirectangularBilinear( const glm::vec3& dir ) const;

    std::string name;
    int width              = 0;
    int height             = 0;
    glm::vec4* pixels      = nullptr;
    ImageFlags flags       = 0;
};


struct ImageCubemapCreateInfo
{
    std::string name;
    std::string flattenedCubemapFilename;
    std::string equirectangularFilename;
    std::string faceFilenames[6];
    ImageFlags flags       = 0;
};


enum FaceIndex
{
    FACE_BACK   = 0,
    FACE_LEFT   = 1,
    FACE_FRONT  = 2,
    FACE_RIGHT  = 3,
    FACE_TOP    = 4,
    FACE_BOTTOM = 5,
};

class ImageCubemap
{
public:
    ImageCubemap() = default;
    ImageCubemap( int size );
    ~ImageCubemap() = default;
    ImageCubemap( ImageCubemap&& src );
    ImageCubemap& operator=( ImageCubemap&& src );

    bool Load( ImageCubemapCreateInfo* createInfo );
    bool SaveAsFlattenedCubemap( const std::string& filename ) const;
    bool SaveIndividualFaces( const std::string& filename ) const;
    bool SaveAsEquirectangular( const std::string& filename ) const;
    
    glm::vec4 GetPixel( int face, int row, int col ) const;
    void SetPixel( int face, int row, int col, const glm::vec4 &pixel );
    void SetPixel( int face, int row, int col, const glm::vec3 &pixel );
    glm::vec4 SampleNearest( const glm::vec3& direction ) const;
    glm::vec4 SampleBilinear( const glm::vec3& direction ) const;

    std::string name;
    Image2D faces[6];
};

glm::vec3 CubemapTexCoordToCartesianDir( int cubeFace, glm::vec2 uv );


} // namespace PG


bool SaveExr( const std::string& filename, int width, int height, glm::vec4* pixels );