#include "ui_system.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/clay_ui.h"
#include "core/lua.hpp"
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

Clay_Sizing CreateSizing( int widthType, float widthVal, int heightType, float heightVal )
{
    Clay_Sizing sizing;
    sizing.width.type = (Clay__SizingType)widthType;
    if ( sizing.width.type == CLAY__SIZING_TYPE_PERCENT )
        sizing.width.size.percent = widthVal;
    else
        sizing.width.size.minMax = { widthVal };

    sizing.height.type = (Clay__SizingType)heightType;
    if ( sizing.height.type == CLAY__SIZING_TYPE_PERCENT )
        sizing.height.size.percent = heightVal;
    else
        sizing.height.size.minMax = { heightVal };

    return sizing;
}

Clay_LayoutConfig CreateLayout() { return {}; }

static PG::Lua::ScriptInstance* s_scriptInstance;

#if USING( ASSET_LIVE_UPDATE )
void AssetLiveUpdateCallback( const AssetManager::AssetLiveUpdateList& updateList )
{
    for ( const auto& assetOldNewPair : updateList )
    {
        if ( s_scriptInstance->scriptAsset == assetOldNewPair.first )
        {
            Script* newScript             = static_cast<Script*>( assetOldNewPair.second );
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

    s_scissorStack.reserve( 16 );
    UI::Text::Init();
    StoreFont( AssetManager::Get<Font>( "arial" ) );

    uint64_t clayRequiredMemory = Clay_MinMemorySize();
    s_clayMemory                = Clay_CreateArenaWithCapacityAndMemory( clayRequiredMemory, malloc( clayRequiredMemory ) );

    Clay_Initialize( s_clayMemory, Clay_Dimensions{ Width(), Height() }, Clay_ErrorHandler{ HandleClayErrors } );
    Clay_SetMeasureTextFunction( MeasureText, nullptr );

    sol::state_view state( Lua::State() );
    sol::table uiNamespace = state.create_named_table( "UI" );

    uiNamespace["SIZING_TYPE_FIT"]         = CLAY__SIZING_TYPE_FIT;
    uiNamespace["SIZING_TYPE_GROW"]        = CLAY__SIZING_TYPE_GROW;
    uiNamespace["SIZING_TYPE_PERCENT"]     = CLAY__SIZING_TYPE_PERCENT;
    uiNamespace["SIZING_TYPE_FIXED"]       = CLAY__SIZING_TYPE_FIXED;
    sol::usertype<Clay_Sizing> sizing_type = uiNamespace.new_usertype<Clay_Sizing>( "Sizing" );
    uiNamespace["CreateSizing"]            = CreateSizing;

    sol::usertype<Clay_Padding> padding_type = uiNamespace.new_usertype<Clay_Padding>( "Padding" );
    padding_type["left"]                     = &Clay_Padding::left;
    padding_type["right"]                    = &Clay_Padding::right;
    padding_type["top"]                      = &Clay_Padding::top;
    padding_type["bottom"]                   = &Clay_Padding::bottom;
    uiNamespace["CreatePadding"]             = []( u16 l, u16 r, u16 t, u16 b ) { return Clay_Padding{ l, r, t, b }; };
    uiNamespace["CreatePadding"]             = []( u16 all ) { return Clay_Padding{ all, all, all, all }; };

    uiNamespace["ALIGN_X_LEFT"]                       = CLAY_ALIGN_X_LEFT;
    uiNamespace["ALIGN_X_RIGHT"]                      = CLAY_ALIGN_X_RIGHT;
    uiNamespace["ALIGN_X_CENTER"]                     = CLAY_ALIGN_X_CENTER;
    uiNamespace["ALIGN_Y_TOP"]                        = CLAY_ALIGN_Y_TOP;
    uiNamespace["ALIGN_Y_BOTTOM"]                     = CLAY_ALIGN_Y_BOTTOM;
    uiNamespace["ALIGN_Y_CENTER"]                     = CLAY_ALIGN_Y_CENTER;
    sol::usertype<Clay_ChildAlignment> alignment_type = uiNamespace.new_usertype<Clay_ChildAlignment>( "ChildAlignment" );
    uiNamespace["CreateAlignment"] = []( u8 x, u8 y ) { return Clay_ChildAlignment{ (Clay_LayoutAlignmentX)x, (Clay_LayoutAlignmentY)y }; };

    uiNamespace["LEFT_TO_RIGHT"]                             = CLAY_LEFT_TO_RIGHT;
    uiNamespace["TOP_TO_BOTTOM"]                             = CLAY_TOP_TO_BOTTOM;
    sol::usertype<Clay_LayoutDirection> layoutDirection_type = uiNamespace.new_usertype<Clay_LayoutDirection>( "LayoutDirection" );

    sol::usertype<Clay_LayoutConfig> layout_type = uiNamespace.new_usertype<Clay_LayoutConfig>( "LayoutConfig" );
    layout_type["sizing"]                        = &Clay_LayoutConfig::sizing;
    layout_type["padding"]                       = &Clay_LayoutConfig::padding;
    layout_type["childGap"]                      = &Clay_LayoutConfig::childGap;
    layout_type["childAlignment"]                = &Clay_LayoutConfig::childAlignment;
    layout_type["layoutDirection"]               = &Clay_LayoutConfig::layoutDirection;
    uiNamespace["CreateLayout"]                  = CreateLayout;

    uiNamespace["Color"] = []( float r, float g, float b, float a ) { return Clay_Color{ r, g, b, a }; };
    sol::usertype<Clay_ElementDeclaration> eDeclatation_type = uiNamespace.new_usertype<Clay_ElementDeclaration>( "ElementDeclaration" );
    eDeclatation_type["id"]                                  = &Clay_ElementDeclaration::id;
    eDeclatation_type["layout"]                              = &Clay_ElementDeclaration::layout;
    eDeclatation_type["backgroundColor"]                     = &Clay_ElementDeclaration::backgroundColor;

    uiNamespace["OpenElement"]      = Clay__OpenElement;
    uiNamespace["ConfigureElement"] = Clay__ConfigureOpenElement;
    uiNamespace["CloseElement"]     = Clay__CloseElement;

    return true;
}

void Shutdown()
{
    delete s_scriptInstance;
    s_scissorStack.clear();
    s_fontIdToFontMap.clear();
    free( s_clayMemory.memory );
    UI::Text::Shutdown();
}

void BootMainMenu()
{
    Script* script = AssetManager::Get<Script>( "main_menu" );

    s_scriptInstance = new Lua::ScriptInstance( script );

    // sol::environment( Lua::State(), sol::create, Lua::State().globals() );
    // if ( !scriptAsset->scriptText.empty() )
    //{
    //     g_LuaState->script( scriptAsset->scriptText, env );
    //     updateFunction = env["Update"];
    // }
    // hasUpdateFunction = updateFunction.valid();
}

void Update()
{
    Clay_SetLayoutDimensions( Clay_Dimensions{ Width(), Height() } );
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling & debug tools
    // Clay_SetPointerState( (Clay_Vector2){ mousePositionX, mousePositionY }, isMouseDown );
    // Optional: Update internal pointer position for handling mouseover / click / touch events - needed for scrolling and debug tools
    // Clay_UpdateScrollContainers( true, (Clay_Vector2){ mouseWheelX, mouseWheelY }, deltaTime );

    Clay_BeginLayout();

    if ( s_scriptInstance && s_scriptInstance->hasUpdateFunction )
        s_scriptInstance->updateFunction();

    // An example of laying out a UI with a fixed width sidebar and flexible width main content
    // CLAY( {
    //    //.id     = CLAY_ID( "OuterContainer" ),
    //    .layout = { .sizing = { CLAY_SIZING_GROW( 0 ), CLAY_SIZING_GROW( 0 ) }, .padding = CLAY_PADDING_ALL( 16 ), .childGap = 16 },
    //    //.backgroundColor = { 0, 250, 0, 255 }
    //} )
    //{
    //    CLAY( {
    //        .id = CLAY_ID( "SideBar" ),
    //        .layout =
    //            {
    //                     .sizing          = { .width = CLAY_SIZING_FIXED( 300 ), .height = CLAY_SIZING_GROW( 0 ) },
    //                     .padding         = CLAY_PADDING_ALL( 16 ),
    //                     .childGap        = 16,
    //                     .layoutDirection = CLAY_TOP_TO_BOTTOM,
    //                     },
    //        .backgroundColor = COLOR_LIGHT
    //    } )
    //  {
    //      CLAY( {
    //          .id              = CLAY_ID( "ProfilePictureOuter" ),
    //          .layout          = { .sizing = { .width = CLAY_SIZING_GROW( 0 ) },
    //                              .padding        = CLAY_PADDING_ALL( 16 ),
    //                              .childGap       = 16,
    //                              .childAlignment = { .y = CLAY_ALIGN_Y_CENTER } },
    //          .backgroundColor = COLOR_RED
    //      } )
    //{
    //    CLAY_TEXT( CLAY_STRING( "Clay - UI Library" ), CLAY_TEXT_CONFIG( {
    //                                                       .textColor = { 255, 255, 255, 255 },
    //                                                         .fontSize = 24
    //    } ) );
    //}
    //
    //      CLAY( { .id          = CLAY_ID( "MainContent" ),
    //          .layout          = { .sizing = { .width = CLAY_SIZING_GROW( 0 ), .height = CLAY_SIZING_GROW( 0 ) } },
    //          .backgroundColor = COLOR_LIGHT } )
    //{
    //    GfxImage* img = AssetManager::Get<GfxImage>( "macaw" );
    //    CLAY( {
    //        .layout = { .sizing = { .width = CLAY_SIZING_FIXED( 60 ), .height = CLAY_SIZING_FIXED( 60 ) } },
    //        .image  = { .imageData = img, .sourceDimensions = { (float)img->width, (float)img->height } }
    //    } )
    //    {
    //    }
    //}
    //  }
    //}
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
            pushConstants.aabb                    = { aabb.x, aabb.y, aabb.width, aabb.height };
            pushConstants.color                   = ClayToPGColor( cRect.backgroundColor );
            pushConstants.textureIndex            = PG_INVALID_TEXTURE_INDEX;
            cmdBuf.PushConstants( pushConstants );
            cmdBuf.DrawMeshTasks( 1, 1, 1 );
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
