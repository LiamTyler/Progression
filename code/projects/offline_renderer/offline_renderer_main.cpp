#include "core/init.hpp"
#include "path_tracer.hpp"
#include "pt_scene.hpp"
#include "shared/filesystem.hpp"
#include "shared/random.hpp"
#include <filesystem>

using namespace PT;
using namespace PG;

int main( int argc, char** argv )
{
    if ( !EngineInitialize() )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }

    if ( argc != 2 )
    {
        LOG( "Usage: pathTracer SCENE_FILE" );
        return 0;
    }

    std::string sceneName = argc > 1 ? argv[1] : "";
    Scene* scene          = new Scene;
    if ( !scene->Load( PG_ASSET_DIR "scenes/" + sceneName + ".json" ) )
    {
        LOG_ERR( "Could not load scene file '%s'", sceneName.c_str() );
        return 0;
    }

    // Perform all scene.numSamplesPerPixel.size() of the renderings.
    // Can specify to render the scene multiple times with different numbers of SPP using "SamplesPerPixel": [ 8, 32, etc... ]
    for ( i32 sppIteration = 0; sppIteration < (i32)scene->settings.numSamplesPerPixel.size(); ++sppIteration )
    {
        PathTracer pathTracer( scene );
        pathTracer.Render( sppIteration );

        // if there are multiple renderings, tack on the suffix "_[spp]" to the filename"
        std::string filename = PG_ROOT_DIR + scene->settings.outputImageFilename;
        if ( scene->settings.numSamplesPerPixel.size() > 1 )
        {
            filename = PG_ROOT_DIR + GetFilenameMinusExtension( filename ) + "_" +
                       std::to_string( scene->settings.numSamplesPerPixel[sppIteration] ) + GetFileExtension( filename );
        }

        if ( pathTracer.SaveImage( filename ) )
        {
            LOG( "Saved path traced image: %s", filename.c_str() );
        }
    }

    delete scene;
    EngineShutdown();

    return 0;
}
