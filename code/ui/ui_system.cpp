#include "ui_system.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/clay_ui.h"
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"
#include "ui_text.hpp"

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"

const Clay_Color COLOR_LIGHT  = Clay_Color{ 224, 215, 210, 255 };
const Clay_Color COLOR_RED    = Clay_Color{ 168, 66, 28, 255 };
const Clay_Color COLOR_ORANGE = Clay_Color{ 225, 138, 50, 255 };

enum : u32
{
    PIPELINE_OPAQUE,
    PIPELINE_BLEND,

    PIPELINE_COUNT
};

static PG::Pipeline* s_pipelines[PIPELINE_COUNT];
Clay_Arena s_clayMemory;
Clay_RenderCommandArray s_clayRenderCommands;
std::unordered_map<u16, PG::Font*> s_fontIdToFontMap;
std::vector<PG::Gfx::Scissor> s_scissorStack;

inline float Width() { return (float)PG::Gfx::rg.displayWidth; }
inline float Height() { return (float)PG::Gfx::rg.displayHeight; }
inline vec4 ClayToPGColor( const Clay_Color& c ) { return vec4( c.r, c.g, c.b, c.a ) / 255.0f; }

namespace PG::UI
{

void HandleClayErrors( Clay_ErrorData errorData ) { LOG_ERR( "Clay error: %s", errorData.errorText.chars ); }

static Font* GetFont( u16 fontID ) { return s_fontIdToFontMap[fontID]; }

static u16 StoreFont( Font* font )
{
    u16 id                = static_cast<u16>( s_fontIdToFontMap.size() );
    s_fontIdToFontMap[id] = font;
    return id;
}

static inline Clay_Dimensions MeasureText( Clay_StringSlice text, Clay_TextElementConfig* config, void* userData )
{
    Font* font = GetFont( config->fontId );
    vec2 dims  = Text::MeasureText( text.chars, text.length, font, config->fontSize );
    return { dims.x, dims.y };
}

bool Init()
{
    if ( !AssetManager::LoadFastFile( "ui_required" ) )
    {
        return false;
    }

    s_pipelines[PIPELINE_OPAQUE] = Gfx::PipelineManager::GetPipeline( "clay_ui_opaque" );
    s_pipelines[PIPELINE_BLEND]  = Gfx::PipelineManager::GetPipeline( "clay_ui_blend" );

    s_scissorStack.reserve( 16 );
    UI::Text::Init();
    StoreFont( AssetManager::Get<Font>( "arial" ) );

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    s_clayMemory                = Clay_CreateArenaWithCapacityAndMemory( clayRequiredMemory, malloc( clayRequiredMemory ) );

    Clay_Initialize( s_clayMemory, Clay_Dimensions{ Width(), Height() }, Clay_ErrorHandler{ HandleClayErrors } );
    Clay_SetMeasureTextFunction( MeasureText, nullptr );

    return true;
}

void Shutdown()
{
    s_scissorStack.clear();
    s_fontIdToFontMap.clear();
    free( s_clayMemory.memory );
    UI::Text::Shutdown();
}

void Update()
{
    Clay_SetLayoutDimensions( Clay_Dimensions{ Width(), Height() } );
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling & debug tools
    // Clay_SetPointerState( (Clay_Vector2){ mousePositionX, mousePositionY }, isMouseDown );
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    // Clay_UpdateScrollContainers( true, (Clay_Vector2){ mouseWheelX, mouseWheelY }, deltaTime );

    Clay_BeginLayout();

    // An example of laying out a UI with a fixed width sidebar and flexible width main content
    CLAY( {
        .id     = CLAY_ID( "OuterContainer" ),
        .layout = { .sizing = { CLAY_SIZING_GROW( 0 ), CLAY_SIZING_GROW( 0 ) }, .padding = CLAY_PADDING_ALL( 16 ), .childGap = 16 },
        .backgroundColor = { 250, 250, 255, 255 }
    } )
    {
        CLAY( {
            .id = CLAY_ID( "SideBar" ),
            .layout =
                {
                         .sizing          = { .width = CLAY_SIZING_FIXED( 300 ), .height = CLAY_SIZING_GROW( 0 ) },
                         .padding         = CLAY_PADDING_ALL( 16 ),
                         .childGap        = 16,
                         .layoutDirection = CLAY_TOP_TO_BOTTOM,
                         },
            .backgroundColor = COLOR_LIGHT
        } )
        {
            CLAY( {
                .id              = CLAY_ID( "ProfilePictureOuter" ),
                .layout          = { .sizing = { .width = CLAY_SIZING_GROW( 0 ) },
                                    .padding        = CLAY_PADDING_ALL( 16 ),
                                    .childGap       = 16,
                                    .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
                .backgroundColor = COLOR_RED
            } )
            {
                CLAY_TEXT( CLAY_STRING( "Clay - UI Library" ), CLAY_TEXT_CONFIG( {
                                                                   .textColor = { 255, 255, 255, 255 },
                                                                     .fontSize = 24
                } ) );
            }

            CLAY( { .id          = CLAY_ID( "MainContent" ),
                .layout          = { .sizing = { .width = CLAY_SIZING_GROW( 0 ), .height = CLAY_SIZING_GROW( 0 ) } },
                .backgroundColor = COLOR_LIGHT } )
            {
                GfxImage* img = AssetManager::Get<GfxImage>( "macaw" );
                CLAY( {
                    .layout = { .sizing = { .width = CLAY_SIZING_FIXED( 60 ), .height = CLAY_SIZING_FIXED( 60 ) } },
                    .image  = { .imageData = img, .sourceDimensions = { (float)img->width, (float)img->height } }
                } )
                {
                }
            }
        }
    }
    // CLAY({ .id = CLAY_ID("ProfilePictureOuter"), .layout = { .sizing = { .width = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(16),
    // .childGap = 16, .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } }, .backgroundColor = COLOR_RED }) { CLAY({ .id =
    // CLAY_ID("ProfilePicture"), .layout = { .sizing = { .width = CLAY_SIZING_FIXED(60), .height = CLAY_SIZING_FIXED(60) }}, .image = {
    // .imageData = &profilePicture, .sourceDimensions = {60, 60} } }) {} CLAY_TEXT(CLAY_STRING("Clay - UI Library"), CLAY_TEXT_CONFIG({
    // .fontSize = 24, .textColor = {255, 255, 255, 255} }));

    s_clayRenderCommands = Clay_EndLayout();
}

void Render( Gfx::CommandBuffer& cmdBuf )
{
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
            pushConstants.pos.x                   = aabb.x;
            pushConstants.pos.y                   = aabb.y;
            pushConstants.dimensions.x            = aabb.width;
            pushConstants.dimensions.y            = aabb.height;
            pushConstants.color                   = ClayToPGColor( cRect.backgroundColor );
            pushConstants.textureIndex            = PG_INVALID_TEXTURE_INDEX;
            cmdBuf.PushConstants( pushConstants );
            cmdBuf.DrawMeshTasks( 1, 1, 1 );
            break;
        }
        case CLAY_RENDER_COMMAND_TYPE_IMAGE:
        {
            const Clay_ImageRenderData& cImage = renderCommand->renderData.image;
            pushConstants.pos.x                = aabb.x;
            pushConstants.pos.y                = aabb.y;
            pushConstants.dimensions.x         = aabb.width;
            pushConstants.dimensions.y         = aabb.height;
            pushConstants.color                = ClayToPGColor( cImage.backgroundColor );
            if ( pushConstants.color == vec4( 0 ) )
                pushConstants.color = vec4( 1 );

            GfxImage* img              = (GfxImage*)cImage.imageData;
            pushConstants.textureIndex = img ? img->gpuTexture.GetBindlessIndex() : PG_INVALID_TEXTURE_INDEX;

            cmdBuf.PushConstants( pushConstants );
            cmdBuf.DrawMeshTasks( 1, 1, 1 );
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
}

} // namespace PG::UI
