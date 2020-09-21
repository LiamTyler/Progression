#include "graphics/graphics_api/ui.hpp"
#include "core/assert.hpp"
#include "core/input.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api/buffer.hpp"
#include "graphics/graphics_api/descriptor.hpp"
#include "graphics/graphics_api/pipeline.hpp"
#include "graphics/graphics_api/texture.hpp"
#include "graphics/vulkan.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"

/**
    Note: Largely taken from Sasha Willem's vulkan imgui code:
    https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanUIOverlay.cpp
*/

extern int g_debugLayer;

namespace PG
{
namespace Gfx
{

static bool s_visible;
static bool s_updated;
static Texture s_fontTexture;
static DescriptorPool s_descriptorPool;
static std::vector< DescriptorSetLayout > s_descriptorSetLayouts;
static DescriptorSet s_descriptorSet;
static Pipeline s_pipeline;
static Buffer s_VBO, s_IBO;
static int s_vertexCount = 0, s_indexCount = 0;
static std::unordered_map< std::string, std::function< void() > > s_drawFunctions;

struct PushConstBlock
{
	glm::vec2 scale;
	glm::vec2 translate;
} pushConstBlock;

namespace UIOverlay
{

    bool Init()
    {
        s_visible = false;
        s_updated = false;
        ImGui::CreateContext();
        ImGuiIO& io                = ImGui::GetIO();
        io.IniFilename             = NULL;
		io.DisplaySize             = ImVec2( (float) GetMainWindow()->Width(), (float) GetMainWindow()->Height() );
		io.DisplayFramebufferScale = ImVec2( 1.0f, 1.0f );

        unsigned char* fontData;
        int texWidth, texHeight;
        io.Fonts->GetTexDataAsRGBA32( &fontData, &texWidth, &texHeight );

        // create texture to copy imgui font into
        ImageDescriptor imageDesc;
        imageDesc.format  = PixelFormat::R8_G8_B8_A8_UNORM;
        imageDesc.width   = texWidth;
        imageDesc.height  = texHeight;
        imageDesc.usage   = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageDesc.sampler = "linear_clamped_linear";
        s_fontTexture = g_renderState.device.NewTextureFromBuffer( imageDesc, fontData, false, "UI Font Texture" );
        if ( !s_fontTexture )
        {
            LOG_ERR( "Could not create ui font texture" );
            return false;
        }

        VertexBindingDescriptor bindingDescs[] =
        {
            VertexBindingDescriptor( 0, sizeof( ImDrawVert ) ),
        };

        VertexAttributeDescriptor attribDescs[] =
        {
            VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT2, offsetof( ImDrawVert, pos ) ),
            VertexAttributeDescriptor( 1, 0, BufferDataType::FLOAT2, offsetof( ImDrawVert, uv ) ),
            VertexAttributeDescriptor( 2, 0, BufferDataType::UCHAR4_NORM, offsetof( ImDrawVert, col ) ),
        };

        auto vertShader = ResourceManager::Get< Shader >( "uiVert" );
        auto fragShader = ResourceManager::Get< Shader >( "uiFrag" );

        std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
        auto combined = CombineDescriptorSetLayouts( descriptorSetData );
        s_descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

	    PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass	                 = &g_renderState.renderPass; // todo: check
        pipelineDesc.descriptorSetLayouts        = s_descriptorSetLayouts;
        pipelineDesc.vertexDescriptor            = VertexInputDescriptor::Create( 1, bindingDescs, 3, attribDescs );
        pipelineDesc.rasterizerInfo.cullFace     = CullFace::NONE;
        pipelineDesc.depthInfo.depthTestEnabled  = false;
        pipelineDesc.depthInfo.depthWriteEnabled = false;
        pipelineDesc.depthInfo.compareFunc       = CompareFunction::ALWAYS;
        pipelineDesc.viewport                    = FullScreenViewport();
        pipelineDesc.scissor                     = FullScreenScissor();
        pipelineDesc.shaders[0]	                 = vertShader.get();
        pipelineDesc.shaders[1]			         = fragShader.get();
        pipelineDesc.dynamicStates               = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        pipelineDesc.colorAttachmentInfos[0].blendingEnabled = true;
        pipelineDesc.colorAttachmentInfos[0].dstAlphaBlendFactor = BlendFactor::ZERO;
        pipelineDesc.colorAttachmentInfos[0].srcAlphaBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;

	    s_pipeline = g_renderState.device.NewPipeline( pipelineDesc , "UIOverlay" );
	    if ( !s_pipeline )
	    {
		    LOG_ERR( "Could not create post UI pipeline" );
		    return false;
	    }

        VkDescriptorPoolSize poolSize = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
        s_descriptorPool = g_renderState.device.NewDescriptorPool( 1, &poolSize, 1, "UIOverlay" );

        s_descriptorSet = s_descriptorPool.NewDescriptorSet( s_descriptorSetLayouts[0], "UI Font texture descriptor layout" );

        VkDescriptorImageInfo imageInfo = DescriptorImageInfo( s_fontTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
        std::vector< VkWriteDescriptorSet > writeDescriptorSets =
        {
            WriteDescriptorSet( s_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageInfo ),
        };
        g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

#if USING( DEBUG_BUILD )
        UIOverlay::AddDrawFunction( "Render Debugger", [&]()
        {
            ImGui::SetNextWindowPos( ImVec2( 5, 5 ), ImGuiCond_FirstUseEver );
		    ImGui::Begin( "Renderer Debug Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize );
            UIOverlay::ComboBox( "View", &g_debugLayer, { "Regular", "No SSAO", "SSAO Only", "Ambient", "Lit Diffuse", "Lit Specular", "Positions", "Normals", "GBuffer Diffuse", "GBuffer Specular" } );
		    ImGui::End();
        });
#endif // #if USING( DEBUG_BUILD )

        return true;
    }

    void Shutdown()
    {
        ImGui::DestroyContext();
        if ( s_VBO )
        {
            s_VBO.Free();
        }
        if ( s_IBO )
        {
            s_IBO.Free();
        }
        s_fontTexture.Free();
        s_pipeline.Free();
        FreeDescriptorSetLayouts( s_descriptorSetLayouts );
        s_descriptorPool.Free();
    }

    static void UpdateBuffers()
    {
        ImDrawData* imDrawData = ImGui::GetDrawData();

		// Note: Alignment is done inside buffer creation
		size_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof( ImDrawVert );
		size_t indexBufferSize  = imDrawData->TotalIdxCount * sizeof( ImDrawIdx );

		if ( vertexBufferSize == 0 || indexBufferSize == 0 )
        {
			return;
		}

		// Update buffers only if vertex or index count has been changed compared to current buffer size

		// Vertex buffer
		if ( !s_VBO || s_vertexCount != imDrawData->TotalVtxCount )
        {
            if ( s_VBO )
            {
                s_VBO.UnMap();
                s_VBO.Free();
            }
            s_VBO = g_renderState.device.NewBuffer( vertexBufferSize, BUFFER_TYPE_VERTEX, MEMORY_TYPE_HOST_VISIBLE, "UI Vertex Buffer" );
            PG_ASSERT( s_VBO );
			s_vertexCount = imDrawData->TotalVtxCount;
            s_VBO.Map();
		}

		// Index buffer
		VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
		if ( !s_IBO || s_indexCount < imDrawData->TotalIdxCount )
        {
            if ( s_IBO )
            {
                s_IBO.UnMap();
                s_IBO.Free();
            }
            s_IBO = g_renderState.device.NewBuffer( indexBufferSize, BUFFER_TYPE_INDEX, MEMORY_TYPE_HOST_VISIBLE, "UI Index Buffer" );
            PG_ASSERT( s_IBO );
			s_indexCount = imDrawData->TotalIdxCount;
            s_IBO.Map();
		}

		// Upload data
		ImDrawVert* vtxDst = reinterpret_cast< ImDrawVert* >( s_VBO.MappedPtr() );
		ImDrawIdx* idxDst  = reinterpret_cast< ImDrawIdx* >( s_IBO.MappedPtr() );

		for ( int n = 0; n < imDrawData->CmdListsCount; n++ )
        {
			const ImDrawList* cmd_list = imDrawData->CmdLists[n];
			memcpy( vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
			memcpy( idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		// Flush to make writes visible to GPU
        s_VBO.Flush();
        s_IBO.Flush();
    }

    void AddDrawFunction( const std::string& name, const std::function< void() >& func )
    {
        s_drawFunctions[name] = func;
    }

    void RemoveDrawFunction( const std::string& name )
    {
        s_drawFunctions.erase( name );
    }
    
    void Draw( CommandBuffer& cmdBuf )
    {
        if ( !s_visible )
        {
            return;
        }

        // update imgui
        ImGuiIO& io     = ImGui::GetIO();
		io.DeltaTime    = Time::DeltaTime();
        auto mousePos   = Input::GetMousePosition();
		io.MousePos     = ImVec2( mousePos.x, mousePos.y );
		io.MouseDown[0] = Input::GetMouseButtonHeld( MouseButton::LEFT );
		io.MouseDown[1] = Input::GetMouseButtonHeld( MouseButton::RIGHT );

        // draw
        ImGui::NewFrame();

        for ( const auto& [ name, func ] : s_drawFunctions )
        {
            func();
        }
        
        // Render to generate draw buffers
		ImGui::Render();

        UpdateBuffers();

        ImDrawData* imDrawData = ImGui::GetDrawData();
		if ( !imDrawData || imDrawData->CmdListsCount == 0 )
        {
			return;
		}

        cmdBuf.BindRenderPipeline( s_pipeline );
        cmdBuf.BindDescriptorSets( 1, &s_descriptorSet, s_pipeline );

        Viewport viewport( ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y );
        cmdBuf.SetViewport( viewport );

		pushConstBlock.scale     = glm::vec2( 2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y );
		pushConstBlock.translate = glm::vec2( -1.0f );
        cmdBuf.PushConstants( s_pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PushConstBlock ), &pushConstBlock );

        cmdBuf.BindVertexBuffer( s_VBO );
        cmdBuf.BindIndexBuffer( s_IBO, IndexType::UNSIGNED_SHORT );

        int vertexOffset = 0;
		int indexOffset  = 0;
		for ( int i = 0; i < imDrawData->CmdListsCount; i++ )
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for ( int j = 0; j < cmd_list->CmdBuffer.Size; j++ )
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
                Scissor scissor;
                scissor.x      = std::max( static_cast< int >( pcmd->ClipRect.x ), 0 );
                scissor.y      = std::max( static_cast< int >( pcmd->ClipRect.y ), 0 );
                scissor.width  = static_cast< int >( pcmd->ClipRect.z - pcmd->ClipRect.x );
                scissor.height = static_cast< int >( pcmd->ClipRect.w - pcmd->ClipRect.y );
                cmdBuf.SetScissor( scissor );
                cmdBuf.DrawIndexed( indexOffset, pcmd->ElemCount, vertexOffset );
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
    }

    bool CapturingMouse()
    {
        return s_visible && ImGui::GetIO().WantCaptureMouse;	
    }

    bool Header( const char* caption )
    {
        return ImGui::CollapsingHeader( caption, ImGuiTreeNodeFlags_DefaultOpen );
    }

	bool CheckBox( const char *caption, bool *value )
	{
		bool res = ImGui::Checkbox( caption, value );
		if ( res ) { s_updated = true; };
		return res;
	}

	bool CheckBox( const char *caption, int *value )
	{
		bool val = ( *value == 1 );
		bool res = ImGui::Checkbox( caption, &val );
		*value = val;
		if ( res ) { s_updated = true; };
		return res;
	}

	bool InputFloat( const char *caption, float *value, float step, uint32_t precision )
	{
		bool res = ImGui::InputFloat( caption, value, step, step * 10.0f, precision );
		if ( res ) { s_updated = true; };
		return res;
	}

	bool SliderFloat( const char* caption, float* value, float min, float max )
	{
		bool res = ImGui::SliderFloat( caption, value, min, max );
		if ( res ) { s_updated = true; };
		return res;
	}

	bool SliderInt(const char* caption, int* value, int min, int max )
	{
		bool res = ImGui::SliderInt( caption, value, min, max );
		if ( res ) { s_updated = true; };
		return res;
	}

	bool ComboBox( const char *caption, int *itemindex, const std::vector< std::string >& items )
	{
		if ( items.empty() )
        {
			return false;
		}
		std::vector< const char* > charitems;
		charitems.reserve( items.size() );
		for ( size_t i = 0; i < items.size(); i++)
        {
			charitems.push_back( items[i].c_str() );
		}
		uint32_t itemCount = static_cast< uint32_t >( charitems.size() );
		bool res = ImGui::Combo( caption, itemindex, &charitems[0], itemCount, itemCount );
		if ( res ) { s_updated = true; };
		return res;
	}

	bool Button( const char *caption )
	{
		bool res = ImGui::Button( caption );
		if ( res ) { s_updated = true; };
		return res;
	}

	void Text( const char *formatstr, ... )
	{
		va_list args;
		va_start( args, formatstr );
		ImGui::TextV( formatstr, args );
		va_end( args );
	}

    bool Visible()
    {
        return s_visible;
    }

    void SetVisible( bool b )
    {
        s_visible = b;
    }

    bool Updated()
    {
        return s_updated;
    }

} // namespace UIOverlay
} // namespace Gfx
} // namespace PG