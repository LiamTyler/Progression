#include "core/init.hpp"
#include "path_tracer.hpp"
#include "pt_scene.hpp"
#include "shared/random.hpp"
#include <filesystem>

using namespace PT;
using namespace PG;
namespace fs = std::filesystem;

int main( int argc, char** argv )
{
    EngineInitInfo engineInitConfig;
    engineInitConfig.offlineRenderer = true;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }
    Random::SetSeed( 0 );

    if ( argc != 2 )
    {
        LOG( "Usage: pathTracer SCENE_FILE" );
        return 0;
    }

    Scene scene;
    if ( !scene.Load( PG_ASSET_DIR + std::string( argv[1] ) ) )
    {
        LOG_ERR( "Could not load scene file '%s'", argv[1] );
        return 0;
    }
    static_assert( std::is_same_v<glm::vec3, glm::vec<3,float>> );

    // Perform all scene.numSamplesPerPixel.size() of the renderings.
    // Can specify to render the scene multiple times with different numbers of SPP using "SamplesPerPixel": [ 8, 32, etc... ]
    for ( int sppIteration = 0; sppIteration < (int)scene.settings.numSamplesPerPixel.size(); ++sppIteration )
    {
        PathTracer pathTracer( &scene );
        pathTracer.Render( sppIteration );

        // if there are multiple renderings, tack on the suffix "_[spp]" to the filename"
        std::string filename = PG_ROOT_DIR + scene.settings.outputImageFilename;
        if ( scene.settings.numSamplesPerPixel.size() > 1 )
        {
            auto path = fs::path( filename );
            filename  = path.stem().string() + "_" + std::to_string( scene.settings.numSamplesPerPixel[sppIteration] ) + path.extension().string();
        }

        if ( !pathTracer.SaveImage( filename, true ) )
        {
            LOG_ERR( "Could not save image '%s'", filename.c_str() );
        }
    }

    EngineShutdown();
    return 0;
}
