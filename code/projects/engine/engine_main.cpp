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


using namespace PG;
using namespace Gfx;

bool g_paused = false;

template <typename T, size_t current>
constexpr int AddEmUp( const T& tuple )
{
    constexpr size_t tupleSize = std::tuple_size_v<T>;
    if constexpr ( tupleSize == current )
    {
        return 0;
    }
    else
    {
        return std::get<current>( tuple ) + AddEmUp<T, current + 1>( tuple );
    }
}

template <typename T>
void AddEmUp( const T& tuple )
{
    int total = AddEmUp<T, 0>( tuple );
    printf( "%d", total );
}


int main( int argc, char* argv[] )
{
    auto t = std::make_tuple( );
    AddEmUp( t );
    
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
    */
    return 0;
}
