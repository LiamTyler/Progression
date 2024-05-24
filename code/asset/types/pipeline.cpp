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

std::string PipelineShaderInfo::GetFinalName( const std::vector<std::string>& defines ) const
{
    size_t hash = 0;
    HashCombine( hash, stage );
    for ( const std::string& define : defines )
        HashCombine( hash, define );

    return name + "_" + std::to_string( hash );
}

bool Pipeline::Load( const BaseAssetCreateInfo* baseInfo )
{
    PG_ASSERT( baseInfo );
    const PipelineCreateInfo* inputCreateInfo = (const PipelineCreateInfo*)baseInfo;
#if USING( CONVERTER )
    createInfo = *inputCreateInfo;
#else  // #if USING( CONVERTER )
    *this = Gfx::PipelineManager::CreatePipeline( *inputCreateInfo );
#endif // #else // #if USING( CONVERTER )
    return true;
}

bool Pipeline::FastfileLoad( Serializer* serializer )
{
#if USING( CONVERTER )
    return false;
#else  // #if USING( CONVERTER )
    using namespace Gfx;
    PipelineCreateInfo createInfo;

    serializer->Read( createInfo.name );
    uint8_t numShaders;
    serializer->Read( numShaders );
    createInfo.shaders.resize( numShaders );
    for ( uint8_t i = 0; i < numShaders; ++i )
    {
        serializer->Read( createInfo.shaders[i].name );
        serializer->Read( createInfo.shaders[i].stage );
    }

    uint8_t numAttachments;
    serializer->Read( numAttachments );
    createInfo.graphicsInfo.colorAttachments.resize( numAttachments );
    for ( uint8_t i = 0; i < numAttachments; ++i )
    {
        PipelineColorAttachmentInfo& aInfo = createInfo.graphicsInfo.colorAttachments[i];
        serializer->Read( &aInfo, sizeof( PipelineColorAttachmentInfo ) );
    }
    serializer->Read( &createInfo.graphicsInfo.rasterizerInfo, sizeof( RasterizerInfo ) );
    serializer->Read( &createInfo.graphicsInfo.depthInfo, sizeof( PipelineDepthInfo ) );
    serializer->Read( &createInfo.graphicsInfo.primitiveType, sizeof( PrimitiveType ) );

    *this = Gfx::PipelineManager::CreatePipeline( createInfo );

    return true;
#endif // #else // #if USING( CONVERTER )
}

bool Pipeline::FastfileSave( Serializer* serializer ) const
{
#if USING( CONVERTER )
    using namespace Gfx;
    serializer->Write( createInfo.name );
    uint8_t numShaders = static_cast<uint8_t>( createInfo.shaders.size() );
    serializer->Write( numShaders );
    for ( const PipelineShaderInfo& shader : createInfo.shaders )
    {
        std::string finalShaderName = shader.GetFinalName( createInfo.defines );
        serializer->Write( finalShaderName );
        serializer->Write( shader.stage );
    }

    uint8_t numAttachments = static_cast<uint8_t>( createInfo.graphicsInfo.colorAttachments.size() );
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
