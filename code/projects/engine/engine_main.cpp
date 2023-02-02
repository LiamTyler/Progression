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
#include "ui/ui_system.hpp"


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
    RenderSystem::CreateTLAS( scene );

    UI::UIElement* element = UI::CreateElement();
    element->blendMode = UI::ElementBlendMode::OPAQUE;
    element->tint = glm::vec4( 1, 1, 1, 0.5f );
    element->pos = glm::vec2( 0.2f, 0.2f );
    element->dimensions = glm::vec2( 0.2f );
    element->image = AssetManager::Get<GfxImage>( "sponza_curtain_green_diff~$default_metalness~13888677538521616104" );

    element = UI::CreateElement( element->Handle() );
    element->blendMode = UI::ElementBlendMode::OPAQUE;
    element->pos = glm::vec2( 0.3f, 0.2f );
    element->image = AssetManager::Get<GfxImage>( "sponza_curtain_blue_diff~$default_metalness~4444517744669486484" );

    element = UI::CreateElement( element->Handle() );
    element->blendMode = UI::ElementBlendMode::OPAQUE;
    element->pos = glm::vec2( 0.4f, 0.2f );
    element->image = AssetManager::Get<GfxImage>( "sponza_curtain_diff~$default_metalness~208185026957753715" );

    element = UI::CreateChildElement( 0, element->Handle() );
    element->blendMode = UI::ElementBlendMode::OPAQUE;
    element->pos = glm::vec2( 0.5f, 0.2f );
    element->image = AssetManager::Get<GfxImage>( "lion~$default_metalness~5063470199971196693" );

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
