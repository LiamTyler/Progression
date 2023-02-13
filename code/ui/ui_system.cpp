#include "ui/ui_system.hpp"
#include "asset/asset_manager.hpp"
#include "core/lua.hpp"
#include "shared/core_defines.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/render_system.hpp"
#include "renderer/r_globals.hpp"
#include "shaders/c_shared/structs.h"
#include "shared/logger.hpp"
#include "shared/platform_defines.hpp"
#include <list>

#if USING( SHIP_BUILD )
#define IF_NOT_SHIP( ... ) do {} while(0)
#else // #if USING( SHIP_BUILD )
#include <bitset>
#define IF_NOT_SHIP( x ) x
#endif // #else // #if USING( SHIP_BUILD )

enum : uint32_t
{
    PIPELINE_OPAQUE,
    PIPELINE_BLEND,
    PIPELINE_ADDITIVE,

    PIPELINE_COUNT
};

static const std::string s_uiPipelineNames[PIPELINE_COUNT] =
{
    "ui2D_opaque",
    "ui2D_blend",
    "ui2D_additive",
};


namespace PG::UI
{

    static constexpr UIElementHandle MAX_UI_ELEMENTS = 4096;
    static UIElement s_uiElements[MAX_UI_ELEMENTS];
    static uint32_t s_uiElementCount;
    static UIElementHandle s_uiNextFreeSlot;

    Gfx::Pipeline s_uiPipelines[PIPELINE_COUNT];
    std::list<UIElementHandle> s_rootUiElements; // TODO: do linked list in statically allocated memory
    static sol::state *s_uiLuaState = nullptr;


    UIElementHandle UIElement::Handle() const
    {
        return static_cast<UIElementHandle>( this - s_uiElements );
    }


    enum class UIScriptFlags : uint32_t
    {
        NONE       = 0,
        HAS_INIT   = (1u << 0),
        HAS_UPDATE = (1u << 1),
    };
    PG_DEFINE_ENUM_OPS( UIScriptFlags );

    class UIScript
    {
    public:
        UIScript() = default;
        UIScript( const std::string& scriptName )
        {
            script = AssetManager::Get<Script>( scriptName );
            PG_ASSERT( script );
            env = sol::environment( *s_uiLuaState, sol::create, s_uiLuaState->globals() );
            s_uiLuaState->script( script->scriptText, env );
        }

        sol::environment env;
        Script* script = nullptr;
    };

    std::vector<UIScript*> s_luaScripts;

    static void RegisterLuaFunctions_UI( lua_State* state )
    {
        sol::state_view lua( state );
        lua["Clear"] = Clear;
        lua["CreateElement"] = CreateElement;
        lua["CreateChildElement"] = CreateChildElement;
        lua["GetElement"] = GetElement;
        lua["RemoveElement"] = RemoveElement;
        lua["CreateLayout"] = CreateLayout;
    }


    bool Init()
    {
        if ( !AssetManager::LoadFastFile( "ui_required" ) )
        {
            return false;
        }

        using namespace Gfx;
        PipelineDescriptor pipelineDesc;    
        pipelineDesc.renderPass = RenderSystem::GetRenderPass( "UI_2D" );
        pipelineDesc.depthInfo.depthTestEnabled  = false;
        pipelineDesc.depthInfo.depthWriteEnabled = false;
        pipelineDesc.rasterizerInfo.winding  = WindingOrder::COUNTER_CLOCKWISE;
        pipelineDesc.rasterizerInfo.cullFace = CullFace::NONE;
        pipelineDesc.vertexDescriptor        = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );
        pipelineDesc.shaders[0]              = AssetManager::Get<Shader>( "uiVert" );
        pipelineDesc.shaders[1]              = AssetManager::Get<Shader>( "uiFrag" );
        for ( uint32_t i = 0; i < PIPELINE_COUNT; ++i )
        {
            ElementBlendMode blendMode = static_cast<ElementBlendMode>( i );
            pipelineDesc.colorAttachmentInfos[0].blendingEnabled = blendMode != ElementBlendMode::OPAQUE;
            if ( blendMode == ElementBlendMode::BLEND )
            {
                pipelineDesc.colorAttachmentInfos[0].srcColorBlendFactor = BlendFactor::SRC_ALPHA;
                pipelineDesc.colorAttachmentInfos[0].dstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
                pipelineDesc.colorAttachmentInfos[0].srcAlphaBlendFactor = BlendFactor::SRC_ALPHA;
                pipelineDesc.colorAttachmentInfos[0].dstAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
                pipelineDesc.colorAttachmentInfos[0].colorBlendEquation = BlendEquation::ADD;
                pipelineDesc.colorAttachmentInfos[0].alphaBlendEquation = BlendEquation::ADD;
            }
            else if ( blendMode == ElementBlendMode::ADDITIVE )
            {
                pipelineDesc.colorAttachmentInfos[0].srcColorBlendFactor = BlendFactor::ONE;
                pipelineDesc.colorAttachmentInfos[0].dstColorBlendFactor = BlendFactor::ONE;
                pipelineDesc.colorAttachmentInfos[0].srcAlphaBlendFactor = BlendFactor::ONE;
                pipelineDesc.colorAttachmentInfos[0].dstAlphaBlendFactor = BlendFactor::ONE;
                pipelineDesc.colorAttachmentInfos[0].colorBlendEquation = BlendEquation::ADD;
                pipelineDesc.colorAttachmentInfos[0].alphaBlendEquation = BlendEquation::ADD;
            }
            s_uiPipelines[i] = r_globals.device.NewGraphicsPipeline( pipelineDesc, s_uiPipelineNames[i] );
            if ( !s_uiPipelines[i] )
                return false;
        }

        Clear();

        AssetManager::LoadFastFile( "mkdd" );

        PG_ASSERT( !s_uiLuaState );
        s_uiLuaState = new sol::state;
        Lua::SetupStateFunctions( *s_uiLuaState );
        RegisterLuaFunctions_UI( *s_uiLuaState );
        AddScript( "ui_startup" );

        return true;
    }


    void Shutdown()
    {
        s_uiElementCount = 0;
        s_rootUiElements.clear();
        Gfx::r_globals.device.WaitForIdle();
        for ( int i = 0; i < PIPELINE_COUNT; ++i )
        {
            s_uiPipelines[i].Free();
        }
        delete s_uiLuaState;
    }


    void Clear()
    {
        s_uiNextFreeSlot = 0;
        s_uiElementCount = 0;
        IF_NOT_SHIP( memset( s_uiElements, 0, sizeof( s_uiElements ) ) );
        s_rootUiElements.clear();
    }


    static bool IsUIElementHandleValid( UIElementHandle handle )
    {
        return handle < MAX_UI_ELEMENTS && IsSet( s_uiElements[handle].flags, UIElementFlags::ACTIVE );
    }


    static void AddRootElement( UIElementHandle handle )
    {
        s_rootUiElements.push_back( handle );
    }


    static void RemoveRootElement( UIElementHandle handle )
    {
        for ( auto it = s_rootUiElements.begin(); it != s_rootUiElements.end(); ++it )
        {
            if ( *it == handle )
            {
                s_rootUiElements.erase( it );
                return;
            }
        }
        PG_ASSERT( false, "Trying to remove an element from root list, that isn't actually a root" );
    }


    static UIElementHandle FindNextFreeSlot( UIElementHandle startSlot )
    {
        ++startSlot;
        for ( ; startSlot < MAX_UI_ELEMENTS; ++startSlot )
        {
            if ( !IsSet( s_uiElements[startSlot].flags, UIElementFlags::ACTIVE ) )
            {
                return startSlot;
            }
        }

        return UI_NULL_HANDLE;
    }


    static UIElement* AllocateAndInitNewElement( UIElementHandle templateElement = UI_NULL_HANDLE )
    {
        UIElementHandle handle = s_uiNextFreeSlot;
        s_uiNextFreeSlot = FindNextFreeSlot( s_uiNextFreeSlot );
        PG_ASSERT( handle < MAX_UI_ELEMENTS );
        PG_ASSERT( !IsSet( s_uiElements[handle].flags, UIElementFlags::ACTIVE ) );

        ++s_uiElementCount;
        UIElement* element = &s_uiElements[handle];
        if ( templateElement != UI_NULL_HANDLE )
        {
            PG_ASSERT( IsUIElementHandleValid( templateElement ) );
            const UIElement *t = GetElement( templateElement );
            element->flags = t->flags;
            element->blendMode = t->blendMode;
            element->pos = t->pos;
            element->dimensions = t->dimensions;
            element->tint = t->tint;
            element->image = t->image;
        }
        else
        {
            element->flags |= UIElementFlags::ACTIVE;
            element->flags |= UIElementFlags::VISIBLE;
        }

        return element;
    }


    UIElement* CreateElement( UIElementHandle templateElement )
    {
        if ( s_uiElementCount >= MAX_UI_ELEMENTS )
        {
            LOG_ERR( "UI::AddElement: Already at the max number of UI elements %u, skipping", s_uiElementCount );
            return nullptr;
        }
        UIElement* element = AllocateAndInitNewElement( templateElement );
        element->parent = UI_NULL_HANDLE;
        element->prevSibling = UI_NULL_HANDLE;
        element->nextSibling = UI_NULL_HANDLE;
        element->firstChild = UI_NULL_HANDLE;
        element->lastChild = UI_NULL_HANDLE;
        AddRootElement( element->Handle() );

        return element;
    }


    // minor optimization, to bypass adding the fresh element to the root element list
    UIElement* CreateChildElement( UIElementHandle parentHandle, UIElementHandle templateElement )
    {
        if ( s_uiElementCount >= MAX_UI_ELEMENTS )
        {
            LOG_ERR( "UI::AddElement: Already at the max number of UI elements %u, skipping", s_uiElementCount );
            return nullptr;
        }
        UIElement* parent = GetElement( parentHandle );
        UIElement* child = AllocateAndInitNewElement( templateElement );
        UIElementHandle childHandle = child->Handle();
        child->parent = parentHandle;
        child->firstChild = UI_NULL_HANDLE;
        child->lastChild = UI_NULL_HANDLE;
        child->nextSibling = UI_NULL_HANDLE;
        if ( parent->firstChild == UI_NULL_HANDLE )
        {
            parent->firstChild = childHandle;
            parent->lastChild = childHandle;
            child->prevSibling = UI_NULL_HANDLE;
        }
        else
        {
            UIElementHandle curLastHandle = parent->lastChild;
            UIElement* curLast = GetElement( curLastHandle );
            parent->lastChild = childHandle;
            curLast->nextSibling = childHandle;
            child->prevSibling = curLastHandle;
        }

        return child;
    }


    void AddChild( UIElementHandle parentHandle, UIElementHandle childHandle )
    {
        UIElement* parent = GetElement( parentHandle );
        UIElement* child = GetElement( childHandle );
        PG_ASSERT( child->parent == UI_NULL_HANDLE, "Re-parenting elements is not supported. Can only assign fresh or root elements to a parent" );
        RemoveRootElement( childHandle );
        child->parent = parentHandle;
        if ( parent->firstChild == UI_NULL_HANDLE )
        {
            parent->firstChild = childHandle;
            parent->lastChild = childHandle;
        }
        else
        {
            UIElementHandle curLastHandle = parent->lastChild;
            UIElement* curLast = GetElement( curLastHandle );
            parent->lastChild = childHandle;
            curLast->nextSibling = childHandle;
            child->prevSibling = curLastHandle;
        }
    }


    UIElement* GetElement( UIElementHandle handle )
    {
        PG_ASSERT( IsUIElementHandleValid( handle ) );
        return &s_uiElements[handle];
    }


    static void RemoveElementHelper_Free( UIElement* e )
    {
        e->flags &= ~UIElementFlags::ACTIVE;
        s_uiNextFreeSlot = std::min( s_uiNextFreeSlot, e->Handle() );
        --s_uiElementCount;
    }


    static void RemoveElementHelper_IgnoreParent( UIElement* e )
    {
        if ( e->prevSibling != UI_NULL_HANDLE )
        {
            GetElement( e->prevSibling )->nextSibling = e->nextSibling;
        }
        if ( e->nextSibling != UI_NULL_HANDLE )
        {
            GetElement( e->nextSibling )->prevSibling = e->prevSibling;
        }

        for ( UIElementHandle childHandle = e->firstChild; childHandle != UI_NULL_HANDLE; childHandle = e->nextSibling )
        {
            RemoveElementHelper_IgnoreParent( GetElement( childHandle ) );
        }

        RemoveElementHelper_Free( e );
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
            RemoveRootElement( handle );
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

        for ( UIElementHandle childHandle = e->firstChild; childHandle != UI_NULL_HANDLE; childHandle = e->nextSibling )
        {
            RemoveElementHelper_IgnoreParent( GetElement( childHandle ) );
        }

        RemoveElementHelper_Free( e );
    }


    static UIElementHandle AllocateContiguousElements( uint32_t numElements )
    {
        if ( s_uiElementCount + numElements >= MAX_UI_ELEMENTS )
        {
            LOG_ERR( "UI::EnterLayout: Creating this layout would push us over MAX_UI_ELEMENTS limit (%u)", MAX_UI_ELEMENTS );
            return UI_NULL_HANDLE;
        }

        uint32_t count = 1;
        uint32_t startSlot = s_uiNextFreeSlot;
        while ( count < numElements && (startSlot + count) < MAX_UI_ELEMENTS )
        {
            if ( !IsSet( s_uiElements[startSlot + count].flags, UIElementFlags::ACTIVE ) )
            {
                ++count;
            }
            else
            {
                count = 1;
                startSlot = FindNextFreeSlot( startSlot + count );
            }
        }
        if ( startSlot >= MAX_UI_ELEMENTS || count != numElements )
        {
            LOG_ERR( "UI::AllocateContiguousElements: there exists no contiguous region in element buffer of size %u", numElements );
        }

        return startSlot;
    }

    
    static void OffsetHandles( UIElement& e, UIElementHandle offset )
    {
        e.parent = e.parent == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.parent + offset;
        e.prevSibling = e.prevSibling == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.prevSibling + offset;
        e.nextSibling = e.nextSibling == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.nextSibling + offset;
        e.firstChild = e.firstChild == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.firstChild + offset;
        e.lastChild = e.lastChild == UI_NULL_HANDLE ? UI_NULL_HANDLE : e.lastChild + offset;
    }


    void CreateLayout( const std::string& layoutName )
    {
        const UILayout* layout = AssetManager::Get<UILayout>( layoutName );
        PG_ASSERT( layout );
        UIElementHandle rootElementHandle = AllocateContiguousElements( (uint32_t)layout->createInfos.size() );
        if ( rootElementHandle == UI_NULL_HANDLE )
            return;

        AddRootElement( rootElementHandle );
        OffsetHandles( s_uiElements[rootElementHandle], rootElementHandle );

        for ( UIElementHandle localHandle = 0; localHandle < (UIElementHandle)layout->createInfos.size(); ++localHandle )
        {
            UIElement& element = s_uiElements[rootElementHandle + localHandle];
            const UIElementCreateInfo& createInfo = layout->createInfos[localHandle];
            element = createInfo.element;
            element.flags |= UIElementFlags::ACTIVE;
            OffsetHandles( element, rootElementHandle );
            if ( !createInfo.imageName.empty() )
            {
                element.image = AssetManager::Get<GfxImage>( createInfo.imageName );
            }
        }
    }


    void AddScript( const std::string& scriptName )
    {
        UIScript* script = new UIScript( scriptName );
        s_luaScripts.emplace_back( script );
        sol::function startFunc = script->env["Start"];
        if ( startFunc.valid() )
        {
            CHECK_SOL_FUNCTION_CALL( startFunc() );
        }
    }


    static uint32_t UNormToByte( float x )
    {
        return static_cast<uint8_t>( 255.0f * x + 0.5f );
    }


    static uint32_t Pack4Unorm( float x, float y, float z, float w )
    {
        uint32_t packed = ( UNormToByte( x ) << 0 ) | ( UNormToByte( y ) << 8 ) | ( UNormToByte( z ) << 16 ) | ( UNormToByte( w ) << 24 );
	    return packed;
    }


    static GpuData::UIElementData GetGpuDataFromUIElement( const UIElement& e )
    {
        GpuData::UIElementData gpuData;
        gpuData.packedTint = Pack4Unorm( e.tint.x, e.tint.y, e.tint.z, e.tint.w );
        gpuData.textureIndex = e.image ? e.image->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;
        gpuData.pos = e.pos;
        gpuData.dimensions = e.dimensions;

        return gpuData;
    }


    static uint32_t GetPipelineForUIElement( const UIElement& element )
    {
        return Underlying( element.blendMode );
    }


    static bool RenderSingleElement( const UIElement& e, Gfx::CommandBuffer* cmdBuf, uint32_t& lastPipelineIndex )
    {
        PG_ASSERT( IsSet( e.flags, UIElementFlags::ACTIVE ) );

        if ( !IsSet( e.flags, UIElementFlags::VISIBLE ) )
            return false;
        
        uint32_t pipelineIdx = GetPipelineForUIElement( e );
        if ( pipelineIdx != lastPipelineIndex )
        {
            lastPipelineIndex = pipelineIdx;
            cmdBuf->BindPipeline( &s_uiPipelines[pipelineIdx] );
        }
        GpuData::UIElementData gpuData = GetGpuDataFromUIElement( e );
        cmdBuf->PushConstants( 0, sizeof( GpuData::UIElementData ), &gpuData );
        cmdBuf->Draw( 0, 6 );

        return true;
    }


    static void RenderElementTree( UIElementHandle handle, Gfx::CommandBuffer* cmdBuf, uint32_t& lastPipelineIndex )
    {
        const UIElement& e = *GetElement( handle );
        if ( RenderSingleElement( e, cmdBuf, lastPipelineIndex ) )
        {
            for ( UIElementHandle childHandle = e.firstChild; childHandle != UI_NULL_HANDLE; childHandle = GetElement( childHandle )->nextSibling )
            {
                RenderElementTree( childHandle, cmdBuf, lastPipelineIndex );
            }
        }
    }


    void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet *bindlessTexturesSet )
    {
        using namespace Gfx;
        cmdBuf->SetViewport( DisplaySizedViewport( false ) );
        cmdBuf->SetScissor( DisplaySizedScissor() );
        cmdBuf->BindPipeline( &s_uiPipelines[PIPELINE_OPAQUE] );
        uint32_t lastPipelineIndex = PIPELINE_OPAQUE;
        cmdBuf->BindDescriptorSet( *bindlessTexturesSet, PG_BINDLESS_TEXTURE_SET );

        for ( auto it = s_rootUiElements.cbegin(); it != s_rootUiElements.cend(); ++it )
        {
            RenderElementTree( *it, cmdBuf, lastPipelineIndex );
        }
    }

} // namespace PG::UI