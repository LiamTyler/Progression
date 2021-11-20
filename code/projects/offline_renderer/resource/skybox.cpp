#include "resource/skybox.hpp"
#include "configuration.hpp"
#include "stb_image/stb_image.h"
#include "utils/logger.hpp"
#include <algorithm>
#include <vector>

namespace PT
{

Skybox::~Skybox()
{
    if ( m_pixels )
    {
        free( m_pixels );
    }
}

bool Skybox::Load( const SkyboxCreateInfo& info )
{
    name = info.name;
    std::vector< std::string > filenames( 6 );
    filenames[0] = RESOURCE_DIR + info.right;
    filenames[1] = RESOURCE_DIR + info.left;
    filenames[2] = RESOURCE_DIR + info.top;
    filenames[3] = RESOURCE_DIR + info.bottom;
    filenames[4] = RESOURCE_DIR + info.back;
    filenames[5] = RESOURCE_DIR + info.front;

    std::vector< float* > pixelData( 6 );
    for ( size_t i = 0; i < 6; ++i )
    {
        stbi_set_flip_vertically_on_load( info.flipVertically );
        stbi_ldr_to_hdr_scale( 1.0f );
        stbi_ldr_to_hdr_gamma( 1.0f );
        int w, h, nc;
        pixelData[i] = stbi_loadf( filenames[i].c_str(), &w, &h, &nc, 4 );

        if ( !pixelData[i] )
        {
            LOG_ERR( "Failed to load image '", filenames[i], "'" );
            return false;
        }
        if ( i == 0 )
        {
            m_width         = w;
            m_height        = h;
        }
        assert( w == m_width && h == m_height );
    }

    size_t pixelsPerFace = m_width * m_height;
    size_t totalPixels   = 6 * pixelsPerFace;
    m_pixels         = static_cast< glm::vec4* >( malloc( totalPixels * sizeof( glm::vec4 ) ) );
    for ( size_t i = 0; i < 6; ++i )
    {
        memcpy( m_pixels + i * pixelsPerFace, pixelData[i], pixelsPerFace * sizeof( glm::vec4 ) );
    }

    return true;
}
    
int Skybox::GetWidth() const
{
    return m_width;
}

int Skybox::GetHeight() const
{
    return m_height;
}

glm::vec4* Skybox::GetPixels() const
{
    return m_pixels;
}

glm::vec4 Skybox::GetPixel( int face, int r, int c ) const
{
    return m_pixels[face * m_width * m_height + r * m_width + c];
}

// https://www.gamedev.net/forums/topic/687535-implementing-a-cube-map-lookup-function/
glm::vec4 Skybox::GetPixel( const Ray& ray ) const
{
    const auto& v    = ray.direction;
    glm::vec3 absDir = glm::abs( ray.direction );
    int faceIndex;
	float ma;
	glm::vec2 uv;

	if ( absDir.z >= absDir.x && absDir.z >= absDir.y )
	{
		faceIndex = v.z < 0.0f ? 5 : 4;
		ma        = 0.5f / absDir.z;
		uv        = glm::vec2( v.z < 0.0 ? -v.x : v.x, v.y );
	}
	else if ( absDir.y >= absDir.x )
	{
		faceIndex = v.y < 0.0f ? 3 : 2;
		ma        = 0.5f / absDir.y;
		uv        = glm::vec2( v.x, v.y < 0.0 ? -v.z : -v.z );
	}
	else
	{
		faceIndex = v.x < 0.0f ? 1 : 0;
		ma        = 0.5f / absDir.x;
		uv        = glm::vec2( v.x < 0.0f ? v.z : -v.z, v.y );
	}
	uv = uv * ma + glm::vec2( 0.5f );

    int w = static_cast< int >( uv.x * m_width );
    int h = static_cast< int >( uv.y * m_height );
    w     = std::min( w, m_width - 1 );
    h     = std::min( h, m_height - 1 );
    
    return GetPixel( faceIndex, h, w );
}

} // namespace PT