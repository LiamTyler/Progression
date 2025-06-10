#include "ui_system.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/clay_ui.h"
#include "clay/clay.h"
#include "core/lua.hpp"
#include "renderer/debug_marker.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"
#include "ui_clay_lua.hpp"
#include "ui_text.hpp"

enum : u32
{
    PIPELINE_OPAQUE,
    PIPELINE_BLEND,

    PIPELINE_COUNT
};

static PG::Pipeline* s_pipelines[PIPELINE_COUNT];
static Clay_RenderCommandArray s_clayRenderCommands;
static std::vector<PG::Gfx::Scissor> s_scissorStack;

inline float Width() { return (float)PG::Gfx::rg.displayWidth; }
inline float Height() { return (float)PG::Gfx::rg.displayHeight; }
inline vec4 ClayToPGColor( const Clay_Color& c ) { return vec4( c.r, c.g, c.b, c.a ) / 255.0f; }

namespace PG::UI
{

static PG::Lua::ScriptInstance* s_scriptInstance;

#if USING( ASSET_LIVE_UPDATE )
void AssetLiveUpdateCallback( const AssetManager::AssetLiveUpdateList& updateList )
{
    for ( const auto& assetOldNewPair : updateList )
    {
        if ( s_scriptInstance->scriptAsset == assetOldNewPair.first )
        {
            Script* newScript             = static_cast<Script*>( assetOldNewPair.second.asset );
            s_scriptInstance->scriptAsset = newScript;
            if ( !newScript->scriptText.empty() )
            {
                PG::Lua::State().script( newScript->scriptText, s_scriptInstance->env );
                s_scriptInstance->updateFunction = s_scriptInstance->env["Update"];
            }
            s_scriptInstance->hasUpdateFunction = s_scriptInstance->updateFunction.valid();
        }
    }
}
#endif // #if USING( ASSET_LIVE_UPDATE )

bool Init()
{
    if ( !AssetManager::LoadFastFile( "ui_required" ) )
    {
        return false;
    }

    s_pipelines[PIPELINE_OPAQUE] = Gfx::PipelineManager::GetPipeline( "clay_ui_opaque" );
    s_pipelines[PIPELINE_BLEND]  = Gfx::PipelineManager::GetPipeline( "clay_ui_blend" );

    if ( !Init_ClayLua() )
        return false;

    s_scissorStack.reserve( 16 );
    UI::Text::Init();

#if USING( ASSET_LIVE_UPDATE )
    AssetManager::AddLiveUpdateCallback( ASSET_TYPE_SCRIPT, AssetLiveUpdateCallback );
#endif // #if USING( ASSET_LIVE_UPDATE )

    return true;
}

void Shutdown()
{
    delete s_scriptInstance;
    s_scissorStack.clear();
    UI::Text::Shutdown();
    Shutdown_ClayLua();
}

void BootMainMenu()
{
    Script* script   = AssetManager::Get<Script>( "main_menu" );
    s_scriptInstance = new Lua::ScriptInstance( script );
}

void Update()
{
    Clay_SetLayoutDimensions( Clay_Dimensions{ Width(), Height() } );
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling & debug tools
    // Clay_SetPointerState( (Clay_Vector2){ mousePositionX, mousePositionY }, isMouseDown );
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    // Clay_UpdateScrollContainers( true, (Clay_Vector2){ mouseWheelX, mouseWheelY }, deltaTime );

    Clay_BeginLayout();

#if 1
    if ( s_scriptInstance && s_scriptInstance->hasUpdateFunction )
        CHECK_SOL_FUNCTION_CALL( s_scriptInstance->updateFunction() );
#else
    // clang-format off
    CLAY( {
        .id = CLAY_ID( "Parent" ),
        .layout          = { .sizing  = { CLAY_SIZING_GROW( 0 ), CLAY_SIZING_GROW( 0 ) },
                            .padding  = CLAY_PADDING_ALL( 16 ),
                            .childGap = 16,
                            .layoutDirection = CLAY_TOP_TO_BOTTOM },
        .backgroundColor = { 0, 200, 20, 255 }
    } )
    {
        CLAY( {
            .id = CLAY_ID( "Side1" ),
            .layout = { .sizing = { CLAY_SIZING_FIXED( 400 ), CLAY_SIZING_GROW( 0 ) }, .padding = { .left = 10, .right = 40, .top = 20, .bottom = 2 } },
            .backgroundColor = { 200, 200, 200, 255 },
            .border = { .color = { 50, 50, 255, 255 }, .width = { .left = 10, .right = 40, .top = 20, .bottom = 2 } }
        } )
        {
            CLAY_TEXT( CLAY_STRING( "Hello" ), CLAY_TEXT_CONFIG({
                .textColor = { 0, 0, 0, 255 },
                .fontId = GetFontByName( "arial" ),
                .fontSize = 16,
                })
            );
        }
        GfxImage* img = AssetManager::Get<GfxImage>( "macaw" );
        CLAY( {
            .id = CLAY_ID( "Side2" ),
            .layout = { .sizing = { CLAY_SIZING_FIXED( 400 ), CLAY_SIZING_GROW( 0 ) } },
            //.backgroundColor = { 128, 128, 210, 255 },
            .image = { .imageData = img }
        } )
        {
        }
    }
    // clang-format on
#endif

    s_clayRenderCommands = Clay_EndLayout();
}

void Render( Gfx::CommandBuffer& cmdBuf )
{
    PGP_ZONE_SCOPEDN( "ClayUIRender" );
    PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdBuf, "ClayUIRender" );
    using namespace Gfx;
    cmdBuf.BindPipeline( s_pipelines[PIPELINE_BLEND] );
    cmdBuf.BindGlobalDescriptors();
    cmdBuf.SetViewport( SceneSizedViewport( false ) );
    s_scissorStack.push_back( SceneSizedScissor() );
    cmdBuf.SetScissor( SceneSizedScissor() );

    GpuData::UIRectElementData pushConstants;
    pushConstants.projMatrix = glm::ortho<float>( 0, Width(), 0, Height() );

    for ( int i = 0; i < s_clayRenderCommands.length; i++ )
    {
        Clay_RenderCommand* renderCommand = &s_clayRenderCommands.internalArray[i];
        Clay_BoundingBox aabb             = renderCommand->boundingBox;

        switch ( renderCommand->commandType )
        {
        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE:
        {
            const Clay_RectangleRenderData& cRect = renderCommand->renderData.rectangle;
            pushConstants.aabb                    = { aabb.x, aabb.y, aabb.width, aabb.height };
            pushConstants.color                   = ClayToPGColor( cRect.backgroundColor );
            pushConstants.textureIndex            = PG_INVALID_TEXTURE_INDEX;
            cmdBuf.PushConstants( pushConstants );
            cmdBuf.DrawMeshTasks( 1, 1, 1 );
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_BORDER:
        {
            const Clay_BorderRenderData& cBorder = renderCommand->renderData.border;
            pushConstants.color                  = ClayToPGColor( cBorder.color );
            pushConstants.textureIndex           = PG_INVALID_TEXTURE_INDEX;

            if ( cBorder.width.left > 0 )
            {
                pushConstants.aabb = { aabb.x, aabb.y + cBorder.cornerRadius.topLeft, cBorder.width.left,
                    aabb.height - cBorder.cornerRadius.topLeft - cBorder.cornerRadius.bottomLeft };
                cmdBuf.PushConstants( pushConstants );
                cmdBuf.DrawMeshTasks( 1, 1, 1 );
            }
            if ( cBorder.width.right > 0 )
            {
                pushConstants.aabb = { aabb.x + aabb.width - cBorder.width.right, aabb.y + cBorder.cornerRadius.topRight,
                    cBorder.width.right, aabb.height - cBorder.cornerRadius.topRight - cBorder.cornerRadius.bottomRight };
                cmdBuf.PushConstants( pushConstants );
                cmdBuf.DrawMeshTasks( 1, 1, 1 );
            }
            if ( cBorder.width.top > 0 )
            {
                pushConstants.aabb = { aabb.x + cBorder.cornerRadius.topLeft, aabb.y,
                    aabb.width - cBorder.cornerRadius.topLeft - cBorder.cornerRadius.topRight, cBorder.width.top };
                cmdBuf.PushConstants( pushConstants );
                cmdBuf.DrawMeshTasks( 1, 1, 1 );
            }
            if ( cBorder.width.bottom > 0 )
            {
                pushConstants.aabb = { aabb.x + cBorder.cornerRadius.bottomLeft, aabb.y + aabb.height - cBorder.width.bottom,
                    aabb.width - cBorder.cornerRadius.bottomLeft - cBorder.cornerRadius.bottomRight, cBorder.width.bottom };
                cmdBuf.PushConstants( pushConstants );
                cmdBuf.DrawMeshTasks( 1, 1, 1 );
            }

            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        {
            const Clay_ImageRenderData& cImage = renderCommand->renderData.image;
            pushConstants.aabb                 = { aabb.x, aabb.y, aabb.width, aabb.height };
            pushConstants.color                = ClayToPGColor( cImage.backgroundColor );
            if ( pushConstants.color == vec4( 0 ) )
                pushConstants.color = vec4( 1 );

            GfxImage* img              = (GfxImage*)cImage.imageData;
            pushConstants.textureIndex = img ? img->gpuTexture.GetBindlessIndex() : PG_INVALID_TEXTURE_INDEX;

            cmdBuf.PushConstants( pushConstants );
            cmdBuf.DrawMeshTasks( 1, 1, 1 );
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_TEXT:
        {
            const Clay_BorderRenderData& cBorder = renderCommand->renderData.border;
            const Clay_TextRenderData& cTextData = renderCommand->renderData.text;
            std::string_view str( cTextData.stringContents.chars, cTextData.stringContents.length );
            UI::Text::TextDrawInfo drawInfo{};
            drawInfo.font     = GetFontById( cTextData.fontId );
            drawInfo.pos      = { aabb.x, aabb.y };
            drawInfo.color    = ClayToPGColor( cTextData.textColor );
            drawInfo.fontSize = cTextData.fontSize;

            UI::Text::Clay_Draw2D( cmdBuf, str, drawInfo );

            cmdBuf.BindPipeline( s_pipelines[PIPELINE_BLEND] );
            cmdBuf.BindGlobalDescriptors();
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
        {
            Scissor newScissor = { (int)roundf( aabb.x ), (int)roundf( aabb.y ), (int)roundf( aabb.width ), (int)roundf( aabb.height ) };
            s_scissorStack.push_back( newScissor );
            cmdBuf.SetScissor( newScissor );
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
        {
            s_scissorStack.pop_back();
            cmdBuf.SetScissor( s_scissorStack.back() );
            break;
        }
        }
    }

    UI::Text::Clay_FinalizeTextDraws();
    PG_DEBUG_MARKER_END_REGION_CMDBUF( cmdBuf );
}

} // namespace PG::UI
