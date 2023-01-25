#include "ui/ui.hpp"
#include "asset/asset_manager.hpp"
#include "renderer/graphics_api.hpp"
#include "renderer/r_globals.hpp"
#include "shaders/c_shared/structs.h"
#include "shared/logger.hpp"
#include "shared/platform_defines.hpp"

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
    static UIElemenet s_uiElements[MAX_UI_ELEMENTS];
    static uint32_t s_uiElementCount;
    static UIElementHandle s_uiElementFreeSlots[MAX_UI_ELEMENTS];
    Gfx::Pipeline s_uiPipelines[PIPELINE_COUNT];

    bool Init( Gfx::RenderPass *uiRenderPass )
    {
        using namespace Gfx;

        Clear();
        if ( !AssetManager::LoadFastFile( "ui_required" ) )
        {
            return false;
        }

        PipelineDescriptor pipelineDesc;    
        pipelineDesc.renderPass = uiRenderPass;
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

        return true;
    }


    void Shutdown()
    {
        s_uiElementCount = 0;
        for ( int i = 0; i < PIPELINE_COUNT; ++i )
        {
            s_uiPipelines[i].Free();
        }
    }


    void Clear()
    {
        for ( UIElementHandle i = 0; i < MAX_UI_ELEMENTS; ++i )
        {
            s_uiElementFreeSlots[i] = MAX_UI_ELEMENTS - 1 - i;
        }
        s_uiElementCount = 0;
        IF_NOT_SHIP( memset( s_uiElements, 0, sizeof( s_uiElements ) ) );
    }


    UIElementHandle AddElement( const UIElemenet &element )
    {
        if ( s_uiElementCount >= MAX_UI_ELEMENTS )
        {
            LOG_ERR( "UI::AddElement: Already at the max number of UI elements %u, skipping", s_uiElementCount );
            return UI_NULL_HANDLE;
        }

        UIElementHandle handle = s_uiElementFreeSlots[MAX_UI_ELEMENTS - 1 - s_uiElementCount];
        PG_ASSERT( handle < MAX_UI_ELEMENTS );
        PG_ASSERT( !IsSet( s_uiElements[handle].flags, UIElementFlags::ACTIVE ) );
        IF_NOT_SHIP( s_uiElementFreeSlots[MAX_UI_ELEMENTS - 1 - s_uiElementCount] = UI_NULL_HANDLE );

        ++s_uiElementCount;
        s_uiElements[handle] = element;
        s_uiElements[handle].flags |= UIElementFlags::ACTIVE;
        return handle;
    }


    UIElemenet* GetElement( UIElementHandle handle )
    {
        PG_ASSERT( handle < MAX_UI_ELEMENTS );
        return &s_uiElements[handle];
    }


    void RemoveElement( UIElementHandle handle )
    {
        PG_ASSERT( s_uiElementCount > 0 );
        PG_ASSERT( handle < MAX_UI_ELEMENTS );
        PG_ASSERT( IsSet( s_uiElements[handle].flags, UIElementFlags::ACTIVE ) );
        s_uiElementFreeSlots[MAX_UI_ELEMENTS - 1 - s_uiElementCount] = handle;
        --s_uiElementCount;
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


    static GpuData::UIElementData GetGpuDataFromUIElement( const UIElemenet& e )
    {
        GpuData::UIElementData gpuData;
        gpuData.packedTint = Pack4Unorm( e.tint.x, e.tint.y, e.tint.z, e.tint.w );
        gpuData.textureIndex = e.image ? e.image->gpuTexture.GetBindlessArrayIndex() : PG_INVALID_TEXTURE_INDEX;
        gpuData.pos = e.pos;
        gpuData.dimensions = e.dimensions;

        return gpuData;
    }


    static uint32_t GetPipelineForUIElement( const UIElemenet& element )
    {
        return Underlying( element.blendMode );
    }


    void Render( Gfx::CommandBuffer* cmdBuf, Gfx::DescriptorSet *bindlessTexturesSet )
    {
        using namespace Gfx;
        cmdBuf->SetViewport( DisplaySizedViewport( false ) );
        cmdBuf->SetScissor( DisplaySizedScissor() );
        cmdBuf->BindPipeline( &s_uiPipelines[PIPELINE_OPAQUE] );
        uint32_t lastPipelineIndex = PIPELINE_OPAQUE;
        cmdBuf->BindDescriptorSet( *bindlessTexturesSet, PG_BINDLESS_TEXTURE_SET );
        uint32_t elementsProcessed = 0;
        uint32_t elementIndex = 0;
        while ( elementsProcessed < s_uiElementCount )
        {
            PG_ASSERT( elementIndex < MAX_UI_ELEMENTS );
            const UIElemenet& e = s_uiElements[elementIndex++];
            if ( !IsSet( e.flags, UIElementFlags::ACTIVE ) )
                continue;

            ++elementsProcessed;
            if ( !IsSet( e.flags, UIElementFlags::VISIBLE ) )
                continue;

            uint32_t pipelineIdx = GetPipelineForUIElement( e );
            if ( pipelineIdx != lastPipelineIndex )
            {
                lastPipelineIndex = pipelineIdx;
                cmdBuf->BindPipeline( &s_uiPipelines[pipelineIdx] );
            }
            GpuData::UIElementData gpuData = GetGpuDataFromUIElement( e );
            cmdBuf->PushConstants( 0, sizeof( GpuData::UIElementData ), &gpuData );
            cmdBuf->Draw( 0, 6 );
        }
    }

} // namespace PG::UI