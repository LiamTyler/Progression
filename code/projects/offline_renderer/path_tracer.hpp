#pragma once

#include "image.hpp"
#include "offline_scene.hpp"

namespace PT
{

class PathTracer
{
public:
    PathTracer( Scene* scene );

    void Render( int samplesPerPixelIteration = 0 );
    bool SaveImage( const std::string& filename, bool tonemap ) const;

    Scene* scene;
    ImageF32 renderedImage;
};

} // namespace PT