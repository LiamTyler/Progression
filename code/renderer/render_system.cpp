#include "render_system.hpp"
#include "asset/asset_manager.hpp"
#include "r_globals.hpp"
#include "r_init.hpp"
#include "taskgraph/r_taskGraph.hpp"

using namespace PG;
using namespace Gfx;

constexpr int TEX_SLOT = 7;

namespace PG::RenderSystem
{

struct DescriptorBuffer
{
    VkDeviceSize layoutSize;
    VkDeviceSize offset;
    Buffer buffer;
};

DescriptorBuffer s_descriptorBuffer;

Texture s_drawImg;
VkDescriptorSetLayout s_setLayout;
VkDescriptorSet s_bindlessSet;
VkPipeline s_gradientPipeline;
VkPipelineLayout s_gradientPipelineLayout;

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding( uint32_t binding, VkDescriptorType type, uint32_t count )
    {
        VkDescriptorSetLayoutBinding newbind{};
        newbind.binding         = binding;
        newbind.descriptorCount = count;
        newbind.descriptorType  = type;

        bindings.push_back( newbind );
    }

    void clear() { bindings.clear(); }

    VkDescriptorSetLayout build( VkDevice device, VkShaderStageFlags shaderStages = VK_SHADER_STAGE_ALL )
    {
        for ( auto& b : bindings )
        {
            b.stageFlags |= shaderStages;
        }

        VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        info.pBindings    = bindings.data();
        info.bindingCount = (uint32_t)bindings.size();
        info.flags        = 0;
        // info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        info.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

        VkDescriptorBindingFlags bindFlag = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
        // bindFlag |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        // bindFlag |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        extendedInfo.bindingCount  = 1;
        extendedInfo.pBindingFlags = &bindFlag;
        info.pNext                 = &extendedInfo;

        VkDescriptorType mutableTexTypes[] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE };

        VkMutableDescriptorTypeListVALVE descTypeList{};
        descTypeList.descriptorTypeCount = ARRAY_COUNT( mutableTexTypes );
        descTypeList.pDescriptorTypes    = mutableTexTypes;

        VkMutableDescriptorTypeCreateInfoEXT mutableTypeInfo{ VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT };
        mutableTypeInfo.mutableDescriptorTypeListCount = 1;
        mutableTypeInfo.pMutableDescriptorTypeLists    = &descTypeList;
        extendedInfo.pNext                             = &mutableTypeInfo;

        VkDescriptorSetLayout setLayout;
        VK_CHECK( vkCreateDescriptorSetLayout( device, &info, nullptr, &setLayout ) );

        return setLayout;
    }
};

bool Init( uint32_t sceneWidth, uint32_t sceneHeight, bool headless )
{
    rg.sceneWidth  = sceneWidth;
    rg.sceneHeight = sceneHeight;

    if ( !R_Init( headless, sceneWidth, sceneHeight ) )
        return false;

    if ( !AssetManager::LoadFastFile( "gfx_required" ) )
        return false;

    TextureCreateInfo texInfo( PixelFormat::R16_G16_B16_A16_FLOAT, sceneWidth, sceneHeight );
    texInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    s_drawImg = rg.device.NewTexture( texInfo, "sceneTex" );

    static constexpr uint32_t MAX_TEXTURES = 1;

    DescriptorLayoutBuilder setLayoutBuilder;
    // setLayoutBuilder.add_binding( 0, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_TEXTURES );
    setLayoutBuilder.add_binding( 0, VK_DESCRIPTOR_TYPE_MUTABLE_EXT, MAX_TEXTURES );
    s_setLayout = setLayoutBuilder.build( rg.device.GetHandle() );

    const auto& dbProps = rg.physicalDevice.GetProperties().dbProps;
    vkGetDescriptorSetLayoutSizeEXT( rg.device, s_setLayout, &s_descriptorBuffer.layoutSize );
    s_descriptorBuffer.layoutSize = ALIGN_UP_POW_2( s_descriptorBuffer.layoutSize, dbProps.descriptorBufferOffsetAlignment );
    vkGetDescriptorSetLayoutBindingOffsetEXT( rg.device, s_setLayout, 0u, &s_descriptorBuffer.offset );

    BufferCreateInfo bCI = {};
    bCI.size             = s_descriptorBuffer.layoutSize;
    bCI.bufferUsage |= BufferUsage::RESOURCE_DESCRIPTOR;
    bCI.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    s_descriptorBuffer.buffer = rg.device.NewBuffer( bCI, "bindless textures descriptor buffer" );

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView   = s_drawImg.GetView();

    VkDescriptorGetInfoEXT image_descriptor_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
    image_descriptor_info.type               = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    image_descriptor_info.data.pSampledImage = &imgInfo;
    size_t mutableDescSize                   = Max( dbProps.sampledImageDescriptorSize, dbProps.storageImageDescriptorSize );
    char* dbPtr                              = s_descriptorBuffer.buffer.GetMappedPtr();
    vkGetDescriptorEXT(
        rg.device, &image_descriptor_info, mutableDescSize, dbPtr + s_descriptorBuffer.offset + TEX_SLOT * mutableDescSize );

    Shader* shader = AssetManager::Get<Shader>( "gradient" );

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount         = 1;
    pipelineLayoutCreateInfo.pSetLayouts            = &s_setLayout;
    VkPushConstantRange pRange                      = { VK_SHADER_STAGE_COMPUTE_BIT, 0, shader->reflectionData.pushConstantSize };
    pipelineLayoutCreateInfo.pushConstantRangeCount = pRange.size ? 1 : 0;
    pipelineLayoutCreateInfo.pPushConstantRanges    = pRange.size ? &pRange : nullptr;
    VK_CHECK( vkCreatePipelineLayout( rg.device, &pipelineLayoutCreateInfo, NULL, &s_gradientPipelineLayout ) );

    VkPipelineShaderStageCreateInfo stageinfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = shader->handle;
    stageinfo.pName  = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.flags  = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
    computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext  = nullptr;
    computePipelineCreateInfo.layout = s_gradientPipelineLayout;
    computePipelineCreateInfo.stage  = stageinfo;

    VK_CHECK( vkCreateComputePipelines( rg.device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &s_gradientPipeline ) );

    return true;
}

void Shutdown()
{
    rg.device.WaitForIdle();
    vkDestroyPipelineLayout( rg.device, s_gradientPipelineLayout, nullptr );
    vkDestroyPipeline( rg.device, s_gradientPipeline, nullptr );
    vkDestroyDescriptorSetLayout( rg.device.GetHandle(), s_setLayout, nullptr );

    s_descriptorBuffer.buffer.Free();
    s_drawImg.Free();
    AssetManager::FreeRemainingGpuResources();
    R_Shutdown();
}

void copy_image_to_image( VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize )
{
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2, .pNext = nullptr };

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount     = 1;
    blitRegion.srcSubresource.mipLevel       = 0;

    blitRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount     = 1;
    blitRegion.dstSubresource.mipLevel       = 0;

    VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
    blitInfo.dstImage       = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage       = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter         = VK_FILTER_LINEAR;
    blitInfo.regionCount    = 1;
    blitInfo.pRegions       = &blitRegion;

    vkCmdBlitImage2( cmd, &blitInfo );
}

void Render()
{
    FrameData& frameData = rg.GetFrameData();
    frameData.renderingCompleteFence.WaitFor();
    frameData.renderingCompleteFence.Reset();
    if ( !rg.headless )
        rg.swapchain.AcquireNextImage( frameData.swapchainSemaphore );

    CommandBuffer& cmdBuf = frameData.primaryCmdBuffer;
    cmdBuf.Reset();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    cmdBuf.TransitionImageLayout( s_drawImg.GetImage(), ImageLayout::UNDEFINED, ImageLayout::GENERAL, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT );

    VkDescriptorBufferBindingInfoEXT pBindingInfos{};
    pBindingInfos.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
    pBindingInfos.address = s_descriptorBuffer.buffer.GetDeviceAddress();
    pBindingInfos.usage   = VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
    vkCmdBindDescriptorBuffersEXT( cmdBuf.GetHandle(), 1, &pBindingInfos );

    uint32_t bufferIndex      = 0; // index into pBindingInfos for vkCmdBindDescriptorBuffersEXT?
    VkDeviceSize bufferOffset = 0;
    vkCmdSetDescriptorBufferOffsetsEXT(
        cmdBuf.GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, s_gradientPipelineLayout, 0, 1, &bufferIndex, &bufferOffset );

    vkCmdBindPipeline( cmdBuf.GetHandle(), VK_PIPELINE_BIND_POINT_COMPUTE, s_gradientPipeline );

    struct ComputePushConstants
    {
        vec4 topColor;
        vec4 botColor;
        uint32_t imageIndex;
    };
    ComputePushConstants push{ vec4( 1, 0, 0, 1 ), vec4( 0, 0, 1, 1 ), TEX_SLOT };
    vkCmdPushConstants(
        cmdBuf.GetHandle(), s_gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( ComputePushConstants ), &push );

    Shader* shader = AssetManager::Get<Shader>( "gradient" );
    vkCmdDispatch( cmdBuf.GetHandle(), (uint32_t)std::ceil( s_drawImg.GetWidth() / (float)shader->reflectionData.workgroupSize.x ),
        (uint32_t)std::ceil( s_drawImg.GetHeight() / (float)shader->reflectionData.workgroupSize.y ), 1 );

    cmdBuf.TransitionImageLayout( s_drawImg.GetImage(), ImageLayout::GENERAL, ImageLayout::TRANSFER_SRC, VK_PIPELINE_STAGE_2_CLEAR_BIT,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT );
    cmdBuf.TransitionImageLayout( rg.swapchain.GetImage(), ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT );

    VkExtent2D swpExt = { rg.swapchain.GetWidth(), rg.swapchain.GetHeight() };
    copy_image_to_image( cmdBuf.GetHandle(), s_drawImg.GetImage(), rg.swapchain.GetImage(), s_drawImg.GetExtent2D(), swpExt );

    cmdBuf.TransitionImageLayout( rg.swapchain.GetImage(), ImageLayout::TRANSFER_DST, ImageLayout::PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT );

    cmdBuf.EndRecording();

    VkSemaphoreSubmitInfo waitInfo =
        SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, frameData.swapchainSemaphore.GetHandle() );
    VkSemaphoreSubmitInfo signalInfo =
        SemaphoreSubmitInfo( VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, frameData.renderingCompleteSemaphore.GetHandle() );
    rg.device.Submit( cmdBuf, &waitInfo, &signalInfo, &frameData.renderingCompleteFence );

    if ( !rg.headless )
        rg.device.Present( rg.swapchain, frameData.renderingCompleteSemaphore );

    ++rg.currentFrameIdx;
}

void CreateTLAS( Scene* scene ) {}
::PG::Gfx::PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name ) { return {}; }

} // namespace PG::RenderSystem
