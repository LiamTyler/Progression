#include "ui/ui_system.hpp"
#include "asset/asset_manager.hpp"
#include "c_shared/structs.h"
#include "c_shared/ui.h"
#include "core/input.hpp"
#include "core/lua.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/render_system.hpp"
#include "shared/core_defines.hpp"
#include "shared/logger.hpp"
#include "shared/platform_defines.hpp"
#include "ui/ui_data_structures.hpp"
#include <list>

enum : uint32_t
{
    PIPELINE_OPAQUE,
    PIPELINE_BLEND,
    PIPELINE_ADDITIVE,

    PIPELINE_COUNT
};

static const std::string s_uiPipelineNames[PIPELINE_COUNT] = {
    "ui2D_opaque",   // PIPELINE_OPAQUE
    "ui2D_blend",    // PIPELINE_BLEND
    "ui2D_additive", // PIPELINE_ADDITIVE
};

namespace PG::UI
{

static constexpr UIElementHandle MAX_UI_ELEMENTS = 4096;
static StaticArrayAllocator<UIElement, UIElementHandle, UI_NULL_HANDLE, MAX_UI_ELEMENTS> s_uiElements;
static StaticArrayAllocator<UIElementFunctions, uint16_t, UI_NULL_HANDLE, MAX_UI_ELEMENTS> s_uiElementFunctions;
static Gfx::Pipeline s_uiPipelines[PIPELINE_COUNT];
static std::list<UIElementHandle> s_rootUiElements; // TODO: do linked list in statically allocated memory
static sol::state* s_uiLuaState = nullptr;
static std::vector<UIScript*> s_luaScripts;
static std::vector<LayoutInfo> s_layouts;

UIElementHandle UIElement::Handle() const { return static_cast<UIElementHandle>( this - s_uiElements.Raw() ); }

static void RegisterLuaFunctions_UI( lua_State* state )
{
    sol::state_view lua( state );
    lua["Clear"]                = Clear;
    lua["CreateElement"]        = CreateElement;
    lua["CreateChildElement"]   = CreateChildElement;
    lua["GetElement"]           = GetElement;
    lua["RemoveElement"]        = RemoveElement;
    lua["CreateLayout"]         = CreateLayout;
    lua["GetLayoutRootElement"] = GetLayoutRootElement;
    lua["GetChildElement"]      = GetChildElement;

    sol::usertype<UIElement> uielement_type = lua.new_usertype<UIElement>( "UIElement" );
    uielement_type["parent"]                = &UIElement::parent;
    uielement_type["prevSibling"]           = &UIElement::prevSibling;
    uielement_type["nextSibling"]           = &UIElement::nextSibling;
    uielement_type["firstChild"]            = &UIElement::firstChild;
    uielement_type["lastChild"]             = &UIElement::lastChild;
    uielement_type["pos"]                   = &UIElement::pos;
    uielement_type["dimensions"]            = &UIElement::dimensions;
    uielement_type["tint"]                  = &UIElement::tint;
    uielement_type.set_function( "Handle", &UIElement::Handle );
    uielement_type.set_function( "SetVisible",
        []( UIElement& e, bool b )
        {
            if ( b )
                e.userFlags |= UIElementUserFlags::VISIBLE;
            else
                e.userFlags &= ~UIElementUserFlags::VISIBLE;
        } );
}

bool Init()
{
    if ( !AssetManager::LoadFastFile( "ui_required" ) )
    {
        return false;
    }

    // using namespace Gfx;
    // PipelineDescriptor pipelineDesc;
    // pipelineDesc.dynamicAttachmentInfo       = RenderSystem::GetPipelineAttachmentInfo( "UI_2D" );
    // pipelineDesc.depthInfo.depthTestEnabled  = false;
    // pipelineDesc.depthInfo.depthWriteEnabled = false;
    // pipelineDesc.rasterizerInfo.winding      = WindingOrder::COUNTER_CLOCKWISE;
    // pipelineDesc.rasterizerInfo.cullFace     = CullFace::NONE;
    // pipelineDesc.vertexDescriptor            = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );
    // pipelineDesc.shaders[0]                  = AssetManager::Get<Shader>( "uiVert" );
    // pipelineDesc.shaders[1]                  = AssetManager::Get<Shader>( "uiFrag" );
    // for ( uint32_t i = 0; i < PIPELINE_COUNT; ++i )
    //{
    //     UIElementBlendMode blendMode                         = static_cast<UIElementBlendMode>( i );
    //     pipelineDesc.colorAttachmentInfos[0].blendingEnabled = blendMode != UIElementBlendMode::OPAQUE;
    //     if ( blendMode == UIElementBlendMode::BLEND )
    //     {
    //         pipelineDesc.colorAttachmentInfos[0].srcColorBlendFactor = BlendFactor::SRC_ALPHA;
    //         pipelineDesc.colorAttachmentInfos[0].dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
    //         pipelineDesc.colorAttachmentInfos[0].srcAlphaBlendFactor = BlendFactor::SRC_ALPHA;
    //         pipelineDesc.colorAttachmentInfos[0].dstAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
    //         pipelineDesc.colorAttachmentInfos[0].colorBlendEquation  = BlendEquation::ADD;
    //         pipelineDesc.colorAttachmentInfos[0].alphaBlendEquation  = BlendEquation::ADD;
    //     }
    //     else if ( blendMode == UIElementBlendMode::ADDITIVE )
    //     {
    //         pipelineDesc.colorAttachmentInfos[0].srcColorBlendFactor = BlendFactor::ONE;
    //         pipelineDesc.colorAttachmentInfos[0].dstColorBlendFactor = BlendFactor::ONE;
    //         pipelineDesc.colorAttachmentInfos[0].srcAlphaBlendFactor = BlendFactor::ONE;
    //         pipelineDesc.colorAttachmentInfos[0].dstAlphaBlendFactor = BlendFactor::ONE;
    //         pipelineDesc.colorAttachmentInfos[0].colorBlendEquation  = BlendEquation::ADD;
    //         pipelineDesc.colorAttachmentInfos[0].alphaBlendEquation  = BlendEquation::ADD;
    //     }
    //     s_uiPipelines[i] = rg.device.NewGraphicsPipeline( pipelineDesc, s_uiPipelineNames[i] );
    //     if ( !s_uiPipelines[i] )
    //         return false;
    // }

    Clear();

    PG_ASSERT( !s_uiLuaState );
    s_uiLuaState = new sol::state;
    Lua::SetupStateFunctions( *s_uiLuaState );
    RegisterLuaFunctions_UI( *s_uiLuaState );

    return true;
}

void BootMainMenu() { AddScript( "ui_startup" ); }

void Shutdown()
{
    Clear();
    Gfx::rg.device.WaitForIdle();
    // for ( uint32_t i = 0; i < PIPELINE_COUNT; ++i )
    //{
    //     s_uiPipelines[i].Free();
    // }
    delete s_uiLuaState;
}

void Clear()
{
    for ( auto s : s_luaScripts )
    {
        delete s;
    }
    s_luaScripts.clear();
    for ( auto& layout : s_layouts )
    {
        delete layout.uiscript;
    }
    s_layouts.clear();
    s_uiElements.Clear();
    for ( int i = 0; i < MAX_UI_ELEMENTS; ++i )
    {
        auto& f = s_uiElementFunctions[i];
        if ( f.update.valid() )
            f.update = sol::lua_nil;
        if ( f.mouseButtonDown.valid() )
            f.mouseButtonDown = sol::lua_nil;
        if ( f.mouseButtonUp.valid() )
            f.mouseButtonUp = sol::lua_nil;
        if ( f.mouseEnter.valid() )
            f.mouseEnter = sol::lua_nil;
        if ( f.mouseLeave.valid() )
            f.mouseLeave = sol::lua_nil;
    }
    s_uiElementFunctions.Clear();
    s_rootUiElements.clear();
}

static bool IsUIElementHandleValid( UIElementHandle handle ) { return handle < MAX_UI_ELEMENTS && s_uiElements.IsSlotInUse( handle ); }

static void AddElementToRootList( UIElementHandle handle ) { s_rootUiElements.push_back( handle ); }

static void RemoveElementFromRootList( UIElementHandle handle )
{
    for ( auto it = s_rootUiElements.cbegin(); it != s_rootUiElements.cend(); ++it )
    {
        if ( *it == handle )
        {
            s_rootUiElements.erase( it );
            return;
        }
    }
}

static UIElementHandle AllocateAndInitNewElement( UIElementHandle templateElement = UI_NULL_HANDLE )
{
    UIElementHandle handle = s_uiElements.AllocOne();
    UIElement* element     = &s_uiElements[handle];
    if ( templateElement != UI_NULL_HANDLE )
    {
        PG_ASSERT( IsUIElementHandleValid( templateElement ) );
        const UIElement* t   = GetElement( templateElement );
        element->userFlags   = t->userFlags;
        element->scriptFlags = t->scriptFlags;
        element->blendMode   = t->blendMode;
        element->pos         = t->pos;
        element->dimensions  = t->dimensions;
        element->tint        = t->tint;
        element->image       = t->image;
    }
    else
    {
        element->userFlags |= UIElementUserFlags::VISIBLE;
    }
    element->scriptFunctionsIdx = UI_NO_SCRIPT_INDEX;

    return handle;
}

template <typename Func, bool VISIBILITY_CHECK = true>
static void IterateElementTree( const UIElementHandle handle, Func ProcessSingleElementFunc )
{
    UIElement& e = *GetElement( handle );
    PG_ASSERT( s_uiElements.IsSlotInUse( handle ) );

    if ( !VISIBILITY_CHECK || IsSet( e.userFlags, UIElementUserFlags::VISIBLE ) )
    {
        ProcessSingleElementFunc( e );

        for ( UIElementHandle childHandle = e.firstChild; childHandle != UI_NULL_HANDLE;
              childHandle                 = GetElement( childHandle )->nextSibling )
        {
            IterateElementTree( childHandle, ProcessSingleElementFunc );
        }
    }
}

template <typename Func, bool VISIBILITY_CHECK = true>
static void IterateAllElementsInOrder( Func ProcessSingleElementFunc )
{
    for ( auto it = s_rootUiElements.cbegin(); it != s_rootUiElements.cend(); ++it )
    {
        IterateElementTree<Func, VISIBILITY_CHECK>( *it, ProcessSingleElementFunc );
    }
}

UIElement* CreateElement( UIElementHandle templateElement )
{
    if ( s_uiElements.Size() >= MAX_UI_ELEMENTS )
    {
        LOG_ERR( "UI::AddElement: Already at the max number of UI elements %u, skipping", s_uiElements.Size() );
        return nullptr;
    }
    UIElementHandle handle = AllocateAndInitNewElement( templateElement );
    UIElement* element     = &s_uiElements[handle];
    element->parent        = UI_NULL_HANDLE;
    element->prevSibling   = UI_NULL_HANDLE;
    element->nextSibling   = UI_NULL_HANDLE;
    element->firstChild    = UI_NULL_HANDLE;
    element->lastChild     = UI_NULL_HANDLE;
    AddElementToRootList( element->Handle() );

    return element;
}

// minor optimization, to bypass adding the fresh element to the root element list
UIElement* CreateChildElement( UIElementHandle parentHandle, UIElementHandle templateElement )
{
    if ( s_uiElements.Size() >= MAX_UI_ELEMENTS )
    {
        LOG_ERR( "UI::AddElement: Already at the max number of UI elements %u, skipping", s_uiElements.Size() );
        return nullptr;
    }
    UIElement* parent           = GetElement( parentHandle );
    UIElementHandle childHandle = AllocateAndInitNewElement( templateElement );
    UIElement* child            = &s_uiElements[childHandle];
    child->parent               = parentHandle;
    child->firstChild           = UI_NULL_HANDLE;
    child->lastChild            = UI_NULL_HANDLE;
    child->nextSibling          = UI_NULL_HANDLE;
    if ( parent->firstChild == UI_NULL_HANDLE )
    {
        parent->firstChild = childHandle;
        parent->lastChild  = childHandle;
        child->prevSibling = UI_NULL_HANDLE;
    }
    else
    {
        UIElementHandle curLastHandle = parent->lastChild;
        UIElement* curLast            = GetElement( curLastHandle );
        parent->lastChild             = childHandle;
        curLast->nextSibling          = childHandle;
        child->prevSibling            = curLastHandle;
    }

    return child;
}

void AddChild( UIElementHandle parentHandle, UIElementHandle childHandle )
{
    UIElement* parent = GetElement( parentHandle );
    UIElement* child  = GetElement( childHandle );
    PG_ASSERT(
        child->parent == UI_NULL_HANDLE, "Re-parenting elements is not supported. Can only assign fresh or root elements to a parent" );
    RemoveElementFromRootList( childHandle );
    child->parent = parentHandle;
    if ( parent->firstChild == UI_NULL_HANDLE )
    {
        parent->firstChild = childHandle;
        parent->lastChild  = childHandle;
    }
    else
    {
        UIElementHandle curLastHandle = parent->lastChild;
        UIElement* curLast            = GetElement( curLastHandle );
        parent->lastChild             = childHandle;
        curLast->nextSibling          = childHandle;
        child->prevSibling            = curLastHandle;
    }
}

UIElement* GetElement( UIElementHandle handle )
{
    PG_ASSERT( IsUIElementHandleValid( handle ) );
    return &s_uiElements[handle];
}

UIElement* GetChildElement( UIElementHandle parentHandle, uint32_t childIdx )
{
    const UIElement& parent     = s_uiElements[parentHandle];
    uint32_t idx                = 0;
    UIElementHandle childHandle = parent.firstChild;
    for ( ; childHandle != UI_NULL_HANDLE && idx < childIdx; childHandle = GetElement( childHandle )->nextSibling )
    {
        ++idx;
    }

    if ( idx == childIdx && childHandle != UI_NULL_HANDLE )
        return &s_uiElements[childHandle];

    return nullptr;
}

static void RemoveElementHelper_IgnoreParent( UIElement* e )
{
    // if ( e->prevSibling != UI_NULL_HANDLE )
    //{
    //     GetElement( e->prevSibling )->nextSibling = e->nextSibling;
    // }
    // if ( e->nextSibling != UI_NULL_HANDLE )
    //{
    //     GetElement( e->nextSibling )->prevSibling = e->prevSibling;
    // }

    for ( UIElementHandle childHandle = e->firstChild; childHandle != UI_NULL_HANDLE; s_uiElements[childHandle].nextSibling )
    {
        RemoveElementHelper_IgnoreParent( GetElement( childHandle ) );
    }

    if ( e->scriptFunctionsIdx != UI_NO_SCRIPT_INDEX )
        s_uiElementFunctions.FreeElement( e->scriptFunctionsIdx );
    s_uiElements.FreeElement( e->Handle() );
}

void RemoveElement( UIElementHandle handle )
{
    UIElement* e = GetElement( handle );
    if ( e->prevSibling != UI_NULL_HANDLE )
    {
        GetElement( e->prevSibling )->nextSibling = e->nextSibling;
    }
    if ( e->nextSibling != UI_NULL_HANDLE )
    {
        GetElement( e->nextSibling )->prevSibling = e->prevSibling;
    }

    if ( e->parent == UI_NULL_HANDLE )
    {
        RemoveElementFromRootList( handle );
    }
    else
    {
        UIElement* parent = GetElement( e->parent );
        if ( parent->firstChild == handle )
        {
            parent->firstChild = e->nextSibling;
        }
        if ( parent->lastChild == handle )
        {
            parent->lastChild = e->prevSibling;
        }
    }

    for ( UIElementHandle childHandle = e->firstChild; childHandle != UI_NULL_HANDLE; childHandle = s_uiElements[childHandle].nextSibling )
    {
        RemoveElementHelper_IgnoreParent( GetElement( childHandle ) );
    }

    if ( e->scriptFunctionsIdx != UI_NO_SCRIPT_INDEX )
    {
        s_uiElementFunctions.FreeElement( e->scriptFunctionsIdx );
    }
    s_uiElements.FreeElement( e->Handle() );
}

UIElement* GetLayoutRootElement( const std::string& layoutName )
{
    UILayout* layoutAsset = AssetManager::Get<UILayout>( layoutName );
    for ( const auto& layout : s_layouts )
    {
        if ( layoutAsset == layout.layoutAsset )
            return GetElement( layout.rootElementHandle );
    }

    return nullptr;
}

static void OffsetHandles( UIElement& e, UIElementHandle offset )
{
    e.parent      = e.parent == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.parent + offset;
    e.prevSibling = e.prevSibling == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.prevSibling + offset;
    e.nextSibling = e.nextSibling == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.nextSibling + offset;
    e.firstChild  = e.firstChild == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.firstChild + offset;
    e.lastChild   = e.lastChild == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.lastChild + offset;
}

static uint16_t AddScript( UIScript* script )
{
    uint16_t idx = static_cast<uint16_t>( s_luaScripts.size() );
    s_luaScripts.emplace_back( script );
    Lua::RunFunctionSafeChecks( script->env, "Start" );

    return idx;
}

uint16_t AddScript( const std::string& scriptName ) { return AddScript( new UIScript( s_uiLuaState, scriptName ) ); }

static void GetFunctionRef(
    const char* funcTypeStr, sol::function& func, const std::string& funcName, UIScript* uiScript, const std::string& scriptName )
{
    if ( funcName.empty() )
    {
        func = sol::lua_nil;
        return;
    }

    func = uiScript->env[funcName];
    if ( !func.valid() )
        LOG_ERR( "Script %s does not have a %s function '%s'", scriptName.c_str(), funcTypeStr, funcName.c_str() );
}

static bool CreateLayoutFromAsset( UILayout* layoutAsset )
{
    UIElementHandle rootElementHandle = s_uiElements.AllocMultiple( (uint32_t)layoutAsset->createInfos.size() );
    if ( rootElementHandle == UI_NULL_HANDLE )
    {
        LOG_ERR( "UI::CreateLayout: No contiguous region to fit %u elements. Current element count: %u",
            (uint32_t)layoutAsset->createInfos.size(), s_uiElements.Size() );
        return false;
    }

    AddElementToRootList( rootElementHandle );
    OffsetHandles( s_uiElements[rootElementHandle], rootElementHandle );

    LayoutInfo& layoutInfo       = s_layouts.emplace_back();
    layoutInfo.layoutAsset       = layoutAsset;
    layoutInfo.rootElementHandle = rootElementHandle;
    layoutInfo.elementCount      = (uint16_t)layoutAsset->createInfos.size();

    for ( UIElementHandle localHandle = 0; localHandle < (UIElementHandle)layoutAsset->createInfos.size(); ++localHandle )
    {
        UIElement& element                    = s_uiElements[rootElementHandle + localHandle];
        const UIElementCreateInfo& createInfo = layoutAsset->createInfos[localHandle];
        element                               = createInfo.element;
        element.scriptFunctionsIdx            = UI_NO_SCRIPT_INDEX;
        OffsetHandles( element, rootElementHandle );
        if ( !createInfo.imageName.empty() )
        {
            element.image = AssetManager::Get<GfxImage>( createInfo.imageName );
        }
    }

    // Must run Start function all the elements have been initialized
    if ( layoutAsset->script )
    {
        layoutInfo.uiscript = new UIScript( s_uiLuaState, layoutAsset->script );
        Lua::RunFunctionSafeChecks( layoutInfo.uiscript->env, "Start" );
        const std::string& scriptName = layoutAsset->script->name;

        for ( UIElementHandle localHandle = 0; localHandle < (UIElementHandle)layoutAsset->createInfos.size(); ++localHandle )
        {
            UIElement& element                    = s_uiElements[rootElementHandle + localHandle];
            const UIElementCreateInfo& createInfo = layoutAsset->createInfos[localHandle];
            if ( element.scriptFlags != UIElementScriptFlags::NONE )
            {
                uint16_t idx                  = s_uiElementFunctions.AllocOne();
                element.scriptFunctionsIdx    = idx;
                UIElementFunctions& functions = s_uiElementFunctions[idx];
                functions.uiScript            = layoutInfo.uiscript;

                GetFunctionRef( "update", functions.update, createInfo.updateFuncName, layoutInfo.uiscript, scriptName );
                GetFunctionRef(
                    "mouseButtonDown", functions.mouseButtonDown, createInfo.mouseButtonDownFuncName, layoutInfo.uiscript, scriptName );
                GetFunctionRef(
                    "mouseButtonUp", functions.mouseButtonUp, createInfo.mouseButtonUpFuncName, layoutInfo.uiscript, scriptName );
            }
        }
    }

    return true;
}

bool CreateLayout( const std::string& layoutName )
{
    UILayout* layoutAsset = AssetManager::Get<UILayout>( layoutName );
    if ( !layoutAsset )
    {
        LOG_ERR( "CreateLayout: UILayout '%s' not found", layoutName.c_str() );
        return false;
    }

    return CreateLayoutFromAsset( layoutAsset );
}

void RemoveLayout( const std::string& layoutName )
{
    UILayout* layoutAsset = AssetManager::Get<UILayout>( layoutName );
    for ( size_t layoutIdx = 0; layoutIdx < s_layouts.size(); ++layoutIdx )
    {
        const LayoutInfo& layout = s_layouts[layoutIdx];
        if ( layoutAsset == layout.layoutAsset )
        {
            RemoveElementFromRootList( layout.rootElementHandle );
            RemoveElement( layout.rootElementHandle );
            if ( layout.uiscript )
                delete layout.uiscript;
            std::swap( s_layouts[layoutIdx], s_layouts[s_layouts.size() - 1] );
            s_layouts.pop_back();
        }
    }
    LOG_ERR( "No layout named '%s' currently in use", layoutName.c_str() );
}

#if USING( ASSET_LIVE_UPDATE )
void ReloadScriptIfInUse( Script* oldScript, Script* newScript )
{
    for ( auto script : s_luaScripts )
    {
        if ( script->script == oldScript )
        {
            s_uiLuaState->script( newScript->scriptText, script->env );
            return;
        }
    }

    for ( size_t oldLayoutIdx = 0; oldLayoutIdx < s_layouts.size(); ++oldLayoutIdx )
    {
        const auto& layoutInfo = s_layouts[oldLayoutIdx];
        if ( !layoutInfo.uiscript || oldScript != layoutInfo.uiscript->script )
            continue;

        const std::string& scriptName = newScript->name;
        s_uiLuaState->script( newScript->scriptText, layoutInfo.uiscript->env );
        for ( uint32_t idx = 0; idx < layoutInfo.elementCount; ++idx )
        {
            UIElement* e = GetElement( layoutInfo.rootElementHandle + idx );
            if ( e->scriptFunctionsIdx != UI_NO_SCRIPT_INDEX )
            {
                const UIElementCreateInfo& createInfo = layoutInfo.layoutAsset->createInfos[idx];
                auto& functions                       = s_uiElementFunctions[e->scriptFunctionsIdx];
                GetFunctionRef( "update", functions.update, createInfo.updateFuncName, layoutInfo.uiscript, scriptName );
                GetFunctionRef(
                    "mouseButtonDown", functions.mouseButtonDown, createInfo.mouseButtonDownFuncName, layoutInfo.uiscript, scriptName );
                GetFunctionRef(
                    "mouseButtonUp", functions.mouseButtonUp, createInfo.mouseButtonUpFuncName, layoutInfo.uiscript, scriptName );
            }
        }

        return;
    }
}

void ReloadLayoutIfInUse( UILayout* oldLayout, UILayout* newLayout )
{
    for ( size_t oldLayoutIdx = 0; oldLayoutIdx < s_layouts.size(); ++oldLayoutIdx )
    {
        if ( oldLayout == s_layouts[oldLayoutIdx].layoutAsset )
        {
            if ( !CreateLayoutFromAsset( newLayout ) )
                return;

            LayoutInfo& oldLayoutInfo = s_layouts[oldLayoutIdx];
            LayoutInfo& newLayoutInfo = s_layouts.back();
            newLayoutInfo.layoutAsset = oldLayout; // because the original pointers are preserved by AssetManager
            // update the s_rootUiElements entry in place, since this order actually affects draw order
            for ( auto it = s_rootUiElements.begin(); it != s_rootUiElements.end(); ++it )
            {
                if ( *it == oldLayoutInfo.rootElementHandle )
                {
                    *it = newLayoutInfo.rootElementHandle;
                    break;
                }
            }
            s_rootUiElements.pop_back();
            RemoveElement( oldLayoutInfo.rootElementHandle );
            if ( oldLayoutInfo.uiscript )
                delete oldLayoutInfo.uiscript;
            std::swap( s_layouts[oldLayoutIdx], s_layouts[s_layouts.size() - 1] );
            s_layouts.pop_back();
        }
    }
}
#endif // #if USING( ASSET_LIVE_UPDATE )

static bool ElementContainsPos( const UIElement& e, const vec2& absolutePos )
{
    return e.pos.x <= absolutePos.x && absolutePos.x <= ( e.pos.x + e.dimensions.x ) && e.pos.y <= absolutePos.y &&
           absolutePos.y <= ( e.pos.y + e.dimensions.y );
}

void Update()
{
    auto luaTimeNamespace  = ( *s_uiLuaState )["Time"].get<sol::table>();
    luaTimeNamespace["dt"] = Time::DeltaTime();

    for ( const auto& script : s_luaScripts )
    {
        Lua::RunFunctionSafeChecks( script->env, "Update" );
    }

    IterateAllElementsInOrder(
        [&]( UIElement& e )
        {
            if ( e.scriptFlags == UIElementScriptFlags::NONE )
                return;

            const UIElementFunctions& funcs = s_uiElementFunctions[e.scriptFunctionsIdx];
            funcs.uiScript->env["e"]        = &e;

            if ( IsSet( e.scriptFlags, UIElementScriptFlags::UPDATE ) )
            {
                CHECK_SOL_FUNCTION_CALL( funcs.update() );
            }

            auto window         = GetMainWindow();
            bool mouseInElement = ElementContainsPos( e, Input::GetMousePosition() / vec2( window->Width(), window->Height() ) );
            if ( mouseInElement )
            {
                if ( Input::AnyMouseButtonDown() && IsSet( e.scriptFlags, UIElementScriptFlags::MOUSE_BUTTON_DOWN ) )
                    CHECK_SOL_FUNCTION_CALL( funcs.mouseButtonDown() );
                if ( Input::AnyMouseButtonUp() && IsSet( e.scriptFlags, UIElementScriptFlags::MOUSE_BUTTON_UP ) )
                    CHECK_SOL_FUNCTION_CALL( funcs.mouseButtonUp() );
            }
        } );
}

static uint32_t UNormToByte( float x ) { return static_cast<uint8_t>( 255.0f * x + 0.5f ); }

static uint32_t Pack4Unorm( float x, float y, float z, float w )
{
    uint32_t packed = ( UNormToByte( x ) << 0 ) | ( UNormToByte( y ) << 8 ) | ( UNormToByte( z ) << 16 ) | ( UNormToByte( w ) << 24 );
    return packed;
}

static GpuData::UIElementData GetGpuDataFromUIElement( const UIElement& e )
{
    GpuData::UIElementData gpuData;
    gpuData.flags        = Underlying( e.userFlags );
    gpuData.type         = Underlying( e.type );
    gpuData.packedTint   = Pack4Unorm( e.tint.x, e.tint.y, e.tint.z, e.tint.w );
    gpuData.textureIndex = e.image ? e.image->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;
    gpuData.pos          = e.pos;
    gpuData.dimensions   = e.dimensions;

    return gpuData;
}

static uint32_t GetPipelineForUIElement( const UIElement& element ) { return Underlying( element.blendMode ); }

void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet* bindlessTexturesSet )
{
    using namespace Gfx;
    cmdBuf->SetViewport( DisplaySizedViewport( false ) );
    cmdBuf->SetScissor( DisplaySizedScissor() );
    cmdBuf->BindPipeline( &s_uiPipelines[PIPELINE_OPAQUE] );
    uint32_t lastPipelineIndex = PIPELINE_OPAQUE;
    cmdBuf->BindDescriptorSet( *bindlessTexturesSet, PG_BINDLESS_TEXTURE_DESCRIPTOR_SET );

    IterateAllElementsInOrder(
        [&]( const UIElement& e )
        {
            uint32_t pipelineIdx = GetPipelineForUIElement( e );
            if ( pipelineIdx != lastPipelineIndex )
            {
                lastPipelineIndex = pipelineIdx;
                cmdBuf->BindPipeline( &s_uiPipelines[pipelineIdx] );
            }
            GpuData::UIElementData gpuData = GetGpuDataFromUIElement( e );
            cmdBuf->PushConstants( 0, sizeof( GpuData::UIElementData ), &gpuData );
            cmdBuf->Draw( 0, 6 );
        } );
}

} // namespace PG::UI
