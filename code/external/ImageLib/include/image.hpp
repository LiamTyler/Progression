#pragma once

#include "glm/glm.hpp"
#include <string>
#include <functional>

struct Image2DCreateInfo
{
    std::string filename;
    bool flipVertically = false;
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

    int width              = 0;
    int height             = 0;
    glm::vec4* pixels      = nullptr;
};


struct ImageCubemapCreateInfo
{
    std::string flattenedCubemapFilename;
    std::string equirectangularFilename;
    std::string faceFilenames[6];
    bool flipVertically = false;
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
    // Saves 6 images, named filenamePrefix + _[back/left/front/etc]. + fileEnding
    bool SaveIndividualFaces( const std::string& filenamePrefix, const std::string& fileEnding ) const;
    bool SaveAsEquirectangular( const std::string& filename ) const;
    
    glm::vec4 GetPixel( int face, int row, int col ) const;
    void SetPixel( int face, int row, int col, const glm::vec4 &pixel );
    void SetPixel( int face, int row, int col, const glm::vec3 &pixel );
    glm::vec4 SampleNearest( const glm::vec3& direction ) const;
    glm::vec4 SampleBilinear( const glm::vec3& direction ) const;

    std::string name;
    Image2D faces[6]; // all faces should have same width and height
};

glm::vec3 CubemapTexCoordToCartesianDir( int cubeFace, glm::vec2 uv );

ImageCubemap GenerateIrradianceMap( const ImageCubemap& cubemap, int faceSize = 64 );