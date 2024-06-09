#include "asset/types/pipeline.hpp"
#include "shared/assert.hpp"
#include "shared/hash.hpp"
#include "shared/serializer.hpp"

#if USING( GPU_DATA )
#include "renderer/r_globals.hpp"
#include "renderer/r_pipeline_manager.hpp"
#endif // #if USING( GPU_DATA )

namespace PG
{

bool Pipeline::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const PipelineCreateInfo* inputCreateInfo = (const PipelineCreateInfo*)baseInfo;
#if USING( CONVERTER )
    createInfo = *inputCreateInfo;
    return true;
#endif // #if USING( CONVERTER )
#if USING( GPU_DATA )
    Gfx::PipelineManager::CreatePipeline( this, *inputCreateInfo );
    return true;
#endif // #if USING( GPU_DATA )
    return false;
}

bool Pipeline::FastfileLoad( Serializer* serializer )
{
#if USING( GPU_DATA )
    using namespace Gfx;
    PipelineCreateInfo createInfo;
#if USING( ASSET_NAMES )
    createInfo.name = m_name;
#endif // #if USING( ASSET_NAMES )

    // serializer->Read( createInfo.name );
    u8 numShaders;
    serializer->Read( numShaders );
    createInfo.shaders.resize( numShaders );
    for ( u8 i = 0; i < numShaders; ++i )
    {
        serializer->Read( createInfo.shaders[i].name );
        serializer->Read( createInfo.shaders[i].stage );
    }

    u8 numAttachments;
    serializer->Read( numAttachments );
    createInfo.graphicsInfo.colorAttachments.resize( numAttachments );
    for ( u8 i = 0; i < numAttachments; ++i )
    {
        PipelineColorAttachmentInfo& aInfo = createInfo.graphicsInfo.colorAttachments[i];
        serializer->Read( &aInfo, sizeof( PipelineColorAttachmentInfo ) );
    }
    serializer->Read( &createInfo.graphicsInfo.rasterizerInfo, sizeof( RasterizerInfo ) );
    serializer->Read( &createInfo.graphicsInfo.depthInfo, sizeof( PipelineDepthInfo ) );
    serializer->Read( &createInfo.graphicsInfo.primitiveType, sizeof( PrimitiveType ) );

    Gfx::PipelineManager::CreatePipeline( this, createInfo );

    return true;
#endif // #if USING( GPU_DATA )
    return false;
}

bool Pipeline::FastfileSave( Serializer* serializer ) const
{
#if USING( CONVERTER )
    using namespace Gfx;
    serializer->Write<u16>( createInfo.name.c_str() );
    u8 numShaders = static_cast<u8>( createInfo.shaders.size() );
    serializer->Write( numShaders );
    for ( const PipelineShaderInfo& shader : createInfo.shaders )
    {
        std::string finalShaderName = GetShaderCacheName( shader.name, shader.stage, createInfo.defines );
        serializer->Write( finalShaderName );
        serializer->Write( shader.stage );
    }

    u8 numAttachments = static_cast<u8>( createInfo.graphicsInfo.colorAttachments.size() );
    serializer->Write( numAttachments );
    for ( const PipelineColorAttachmentInfo& aInfo : createInfo.graphicsInfo.colorAttachments )
    {
        serializer->Write( &aInfo, sizeof( PipelineColorAttachmentInfo ) );
    }
    serializer->Write( &createInfo.graphicsInfo.rasterizerInfo, sizeof( RasterizerInfo ) );
    serializer->Write( &createInfo.graphicsInfo.depthInfo, sizeof( PipelineDepthInfo ) );
    serializer->Write( &createInfo.graphicsInfo.primitiveType, sizeof( PrimitiveType ) );

    return true;
#else  // #if USING( CONVERTER )
    return false;
#endif // #else // #if USING( CONVERTER )
}

#if USING( GPU_DATA )
void Pipeline::Free()
{
    using namespace Gfx;
    PG_ASSERT( m_pipeline != VK_NULL_HANDLE && m_pipelineLayout != VK_NULL_HANDLE );
    vkDestroyPipeline( rg.device, m_pipeline, nullptr );
    vkDestroyPipelineLayout( rg.device, m_pipelineLayout, nullptr );
    m_pipeline       = VK_NULL_HANDLE;
    m_pipelineLayout = VK_NULL_HANDLE;
}

VkPipeline Pipeline::GetHandle() const { return m_pipeline; }
VkPipelineLayout Pipeline::GetLayoutHandle() const { return m_pipelineLayout; }
VkPipelineBindPoint Pipeline::GetPipelineBindPoint() const { return m_bindPoint; };
VkShaderStageFlags Pipeline::GetPushConstantShaderStages() const { return m_stageFlags; }
Gfx::PipelineType Pipeline::GetPipelineType() const { return m_pipelineType; }
uvec3 Pipeline::GetWorkgroupSize() const { return m_workgroupSize; }

Pipeline::operator bool() const { return m_pipeline != VK_NULL_HANDLE; }
Pipeline::operator VkPipeline() const { return m_pipeline; }
#endif // #if USING( GPU_DATA )

} // namespace PG
