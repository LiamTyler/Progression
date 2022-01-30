#pragma once

#include "image.hpp"
#include "pt_scene.hpp"

namespace PT
{

class PathTracer
{
public:
    PathTracer( Scene* scene );

    void Render( int samplesPerPixelIteration = 0 );
    bool SaveImage( const std::string& filename ) const;

    Scene* scene;
    ImageF32 renderedImage;
};

} // namespace PT