#include "asset/asset_manager.hpp"
#include "asset/types/model.hpp"
#include "asset/types/shader.hpp"
#include "core/init.hpp"
#include "core/input.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "image.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/rendergraph/r_rendergraph.hpp"
#include "shared/logger.hpp"
#include "shared/math.hpp"
#include "glm/ext/matrix_transform.hpp"


using namespace PG;
using namespace Gfx;

bool g_paused = false;

int main( int argc, char* argv[] )
{
    EngineInitInfo engineInitConfig;
	if ( !EngineInitialize( engineInitConfig ) )
    {
        LOG_ERR( "Failed to initialize the engine" );
        return 1;
    }
    glm::mat4 model( 1 );
    model = glm::rotate( model, 0.0f, glm::vec3( 0, 1, 0 ) );
    model = glm::rotate( model, 90.f / 180.0f * PI, glm::vec3( 1, 0, 0 ) );
    model = glm::rotate( model, 0.0f, glm::vec3( 0, 0, 1 ) );

    glm::vec4 pos = model * glm::vec4( 1, 0, 1, 1 );

    if ( argc < 2 )
    {
        LOG_ERR( "First argument needs to be path to scene file, relative to folder: %s", PG_ASSET_DIR );
        EngineShutdown();
        return 1;
    }

    Scene* scene = Scene::Load( PG_ASSET_DIR + std::string( argv[1] ) );
    if ( !scene )
    {
        EngineShutdown();
        return 1;
    }

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

    EngineShutdown();

    return 0;
}
