#include "engine_init.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "asset/asset_manager.hpp"
#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "utils/logger.hpp"
#include "core/math.hpp"
#include "asset/image.hpp"

using namespace PG;
using namespace Gfx;

bool g_paused = false;


int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );

    ImageCubemapCreateInfo info = {};
    info.equirectangularFilename = PG_ASSET_DIR "images/newport_loft.hdr";
    ImageCubemap cubemap;
    if ( !cubemap.Load( &info ) )
    {
        return 1;
    }

    int irradianceMapSize = 64;
    auto startTime = Time::GetTimePoint();
    ImageCubemap cubemapIrradiance( irradianceMapSize );
    for ( int faceIndex = 0; faceIndex < 6; ++faceIndex )
    {
        LOG( "cubemap face: %d in %.2f seconds", faceIndex, Time::GetDuration( startTime ) / 1000.0f );
        #pragma omp parallel for schedule(dynamic)
        for ( int r = 0; r < irradianceMapSize; ++r )
        {
            for ( int c = 0; c < irradianceMapSize; ++ c )
            {
                glm::vec2 localUV = { (c + 0.5f) / (float)irradianceMapSize, (r + 0.5f) / (float)irradianceMapSize };
                glm::vec3 N = CubemapTexCoordToCartesianDir( faceIndex, localUV );
    
                glm::vec3 up = glm::vec3( 0, 1, 0 );
                if ( N == up )
                {
                    up = glm::vec3( -1, 0, 0 );
                }
                glm::vec3 right = glm::cross( N, up );
                up = glm::cross( right, N );
                    
                glm::vec3 irradiance( 0 );
                float numSamples = 0;
                const float STEP_IN_RADIANS = 1.0f / 180.0f * PI;
                for ( float phi = 0; phi < 2 * PI; phi += STEP_IN_RADIANS )
                {
                    for ( float theta = 0; theta < 0.5f * PI; theta += STEP_IN_RADIANS )
                    {
                        glm::vec3 tangentSpaceDir = { sin( theta ) * cos( phi ), sin( theta ) * sin( phi ), cos( theta ) };
                        glm::vec3 worldSpaceDir   = tangentSpaceDir.x * right + tangentSpaceDir.y * up + tangentSpaceDir.z * N;
                        glm::vec3 radiance = cubemap.SampleBilinear( worldSpaceDir );
                        irradiance += radiance * cos( theta ) * sin( theta );
                        ++numSamples;
                    }
                }
                cubemapIrradiance.SetPixel( faceIndex, r, c, (PI / numSamples) * irradiance );
            }
        }
    }
    cubemapIrradiance.SaveAsFlattenedCubemap( PG_ASSET_DIR "images/flattened_irradiance_small.png" );

    /*
	EngineInitInfo engineInitConfig;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }

    if ( argc < 2 )
    {
        LOG_ERR( "First argument needs to be path to scene file, relative to folder: %s", PG_ASSET_DIR );
        EngineShutdown();
        return 1;
    }

    Scene* scene = Scene::Load( PG_ASSET_DIR + std::string( argv[1] ) );
    if ( !scene )
    {
        return 1;
    }

    {
        Window* window = GetMainWindow();
        window->SetRelativeMouse( true );

        Input::PollEvents();
        Time::Reset();

        while ( !g_engineShutdown )
        {
            window->StartFrame();
            Input::PollEvents();

            if ( Input::GetKeyDown( Key::ESC ) )
            {
                g_engineShutdown = true;
            }

            scene->Update();

            RenderSystem::Render( scene );

            window->EndFrame();
        }

        delete scene;
    }

    EngineShutdown();
    */

    return 0;
}
