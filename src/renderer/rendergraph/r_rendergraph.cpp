#include "r_rendergraph.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include "utils/logger.hpp"
#include <unordered_map>
#include <unordered_set>


namespace PG
{
namespace Gfx
{
    //bool TG_ResourceDesc::operator==( const TG_ResourceDesc& d ) const
    //{
    //    return format == d.format && width == d.width && height == d.height && depth == d.depth &&
    //        arrayLayers == d.arrayLayers && mipLevels == d.mipLevels && clearColor == d.clearColor;
    //}

    bool ElementsMergable( const RG_Element& a, const RG_Element& b )
    {
        return a.type == b.type && a.format == b.format && a.width == b.width && a.height == b.height && a.depth == b.depth && a.arrayLayers == b.arrayLayers && a.mipLevels == b.mipLevels;
    }


    void ResolveSizes( RG_Element& element, const RenderGraphCompileInfo& info )
    {
        element.width = ResolveRelativeSize( info.sceneWidth, info.displayWidth, element.width );
        element.height = ResolveRelativeSize( info.sceneHeight, info.displayHeight, element.height );

        if ( element.mipLevels == AUTO_FULL_MIP_CHAIN() )
        {
            element.mipLevels = 1 + static_cast< int >( log2f( static_cast< float >( std::max( element.width, element.height ) ) ) );
        }
    }


    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        elements.push_back( { name, ResourceType::COLOR_ATTACH, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, clearColor, true } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        elements.push_back( { name, ResourceType::COLOR_ATTACH, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, float clearValue )
    {
        elements.push_back( { name, ResourceType::DEPTH_ATTACH, ResourceState::WRITE, format, width, height, 1, 1, 1, glm::vec4( clearValue ), true } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height )
    {
        elements.push_back( { name, ResourceType::DEPTH_ATTACH, ResourceState::WRITE, format, width, height, 1, 1, 1, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        elements.push_back( { name, ResourceType::TEXTURE, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, clearColor, true } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        elements.push_back( { name, ResourceType::TEXTURE, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name )   { AddColorOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name )   { AddDepthOutput( name, PixelFormat::INVALID, 0, 0, 0 ); }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name ) { AddTextureOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddTextureInput( const std::string& name )
    {
        elements.push_back( { name, ResourceType::TEXTURE, ResourceState::READ_ONLY, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::SetRenderFunction( RenderFunction func ) { renderFunction = func; }


    RenderGraphBuilder::RenderGraphBuilder()
    {
        tasks.reserve( RenderGraph::MAX_TASKS );
    }


    RenderTaskBuilder* RenderGraphBuilder::AddTask( const std::string& name )
    {
        size_t index = tasks.size();
        tasks.push_back( name );
        return &tasks[index];
    }


    void RenderGraphBuilder::SetBackbufferResource( const std::string& name )
    {
        backbuffer = name;
    }


    bool RenderGraphBuilder::Validate() const
    {
        if ( backbuffer.empty() )
        {
            LOG_ERR( "Please designate an output as the image to be displayed via SetBackbufferResource()" );
            return false;
        }

        uint16_t numBuildTasks = static_cast< uint16_t >( tasks.size() );
        std::unordered_map<std::string, uint16_t> logicalOutputs; // name, task created
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            const RenderTaskBuilder& task = tasks[taskIndex];
            if ( !task.renderFunction )
            {
                LOG_ERR( "Task %s is missing a render function!", task.name.c_str() );
                return false;
            }

            std::unordered_set<std::string> inputs;
            std::unordered_set<std::string> outputs;
            uint8_t numColorAttach = 0;
            uint8_t numDepthAttach = 0;
            for ( const RG_Element& element : task.elements )
            {
                const std::string& name = element.name;
                if ( element.state == ResourceState::READ_ONLY )
                {
                    if ( inputs.find( name ) != inputs.end() )
                    {
                        LOG_ERR( "Resource %s used twice as an input to task %s", name.c_str(), task.name.c_str() );
                        return false;
                    }
                    inputs.insert( name );
                }
                else if ( element.state == ResourceState::WRITE )
                {
                    if ( outputs.find( name ) != outputs.end() )
                    {
                        LOG_ERR( "Resource %s used twice as an output to task %s", name.c_str(), task.name.c_str() );
                        return false;
                    }
                    outputs.insert( name );

                    auto it = logicalOutputs.find( name );
                    if ( element.format != PixelFormat::INVALID )
                    {
                        if ( it != logicalOutputs.end() )
                        {
                            const std::string& originalTask = tasks[logicalOutputs[name]].name;
                            LOG_ERR( "Output %s created for a second time in task %s. Initial creation was in task %s. Should only create an output once", name.c_str(), task.name.c_str(), originalTask.c_str() );
                            return false;
                        }
                        logicalOutputs[name] = taskIndex;
                    }
                    else
                    {
                        if ( it == logicalOutputs.end() )
                        {
                            LOG_ERR( "Output %s in task %s has no create info, but was never created before this task.", name.c_str(), task.name.c_str() );
                            return false;
                        }
                    }

                    numColorAttach += element.type == ResourceType::COLOR_ATTACH;
                    numDepthAttach += element.type == ResourceType::DEPTH_ATTACH;
                }

                if ( numColorAttach > MAX_COLOR_ATTACHMENTS )
                {
                    LOG_ERR( "Task %s has %u color attachments, which exceeds the limit of %u", task.name.c_str(), numColorAttach, MAX_COLOR_ATTACHMENTS );
                    return false;
                }
                if ( numDepthAttach > 1 )
                {
                    LOG_ERR( "Task %s has %u depth attachments, but at most 1 is allowed", task.name.c_str(), numDepthAttach );
                    return false;
                }
            }

            for ( const RG_Element& element : task.elements )
            {
                const std::string& name = element.name;
                if ( element.state == ResourceState::READ_ONLY )
                {
                    auto it = logicalOutputs.find( name );
                    if ( it == logicalOutputs.end() )
                    {
                        LOG_ERR( "Resource %s used as input, but no tasks generate it as an output", name.c_str() );
                        return false;
                    }

                    uint16_t createdInTask = it->second;
                    if ( createdInTask >= taskIndex )
                    {
                        LOG_ERR( "Resource %s used as input in task %s before it created as an output in task %s.", name.c_str(), task.name.c_str(), tasks[createdInTask].name.c_str() );
                        return false;
                    }
                }
            }
        }

        return true;
    }


    struct RG_LogicalOutput
    {
        bool Mergable( const RG_LogicalOutput& res ) const
        {
            bool overlapForward  = res.lastTask >= firstTask && res.firstTask <= lastTask;
            bool overlapBackward = lastTask >= res.firstTask && firstTask <= res.lastTask;
            if ( overlapForward || overlapBackward ) return false;
            if ( !ElementsMergable( this->element, res.element ) ) return false;

            // not tracking multiple clears right now, so in a merged sequence, only the earliest task can have a clear
            if ( element.isCleared && res.element.isCleared ) return false;
            if ( firstTask < res.firstTask && res.element.isCleared ) return false;
            else if ( res.firstTask < firstTask && element.isCleared ) return false;

            return true;
        }

        std::string name;
        RG_Element element;
        uint16_t firstTask;
        uint16_t lastTask;
        uint16_t physicalResourceIndex;
    };


    bool RenderGraph::Compile( RenderGraphBuilder& builder, RenderGraphCompileInfo& compileInfo )
    {
        // TODO: re-order tasks for more optimal rendering. For now, keep same order
        // TODO: handle external images
        // TODO: buffers
        // TODO: compute tasks
        // TODO: cull tasks that dont affect the final image
        // TODO: use StoreOp::DONT_CARE when applicable
        // TODO: right now, if a resource is used as an input, the layouts will never be transitioned to WRITE if it's used as an output later
        // TODO: allow merging with multiple clears. Right now, only 1 clear will happen

        assert( builder.tasks.size() <= MAX_TASKS );

        if ( !builder.Validate() )
        {
            LOG_ERR( "RenderGraph::Compile failed: Invalid RenderGraphBuilder!" );
            return false;
        }

        uint16_t numBuildTasks = static_cast< uint16_t >( builder.tasks.size() );
        backBufferName = builder.backbuffer;
        numRenderTasks = numBuildTasks;
        numPhysicalResources = 0;
        stats = {};

        // 1. Create a list of all resources that are (logically) produced by the task graph.
        //    Also resolves scene relatives sizes, and find lifetime of resource
        std::vector< RG_LogicalOutput > logicalOutputs;
        std::unordered_map< std::string, uint16_t > outputNameToLogicalMap;

        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = builder.tasks[taskIndex];
            taskNameToIndexMap[task.name] = taskIndex;

            for ( RG_Element& element : task.elements )
            {
                ResolveSizes( element, compileInfo );

                if ( element.state == ResourceState::WRITE )
                {
                    if ( element.format != PixelFormat::INVALID )
                    {
                        outputNameToLogicalMap[element.name] = static_cast< uint16_t >( logicalOutputs.size() );
                        logicalOutputs.push_back( { element.name, element, taskIndex, taskIndex, USHRT_MAX } );
                    }
                    else
                    {
                        auto it = outputNameToLogicalMap.find( element.name );
                        RG_LogicalOutput& logicalOutput = logicalOutputs[it->second];
                        logicalOutput.lastTask = taskIndex;
                    }
                }
                else if ( element.state == ResourceState::READ_ONLY )
                {
                    auto it = outputNameToLogicalMap.find( element.name );
                    logicalOutputs[it->second].lastTask = std::max( logicalOutputs[it->second].lastTask, taskIndex );
                }
            }
        }
        stats.numLogicalOutputs = static_cast< uint16_t >( logicalOutputs.size() );

        // 2. Before we create physical resources for these, find out if we can merge two "logical" resources
        //    Two resources are mergeable, if their lifetimes do not overlap at all, and have the same resource descriptors.
        //    Eventually want to allow for different imageviews, or different sized images (ex: storing a 720p image inside of an already allocated 1080p that is no longer needed)
        std::vector< RG_LogicalOutput > mergedLogicalOutputs;
        mergedLogicalOutputs.reserve( logicalOutputs.size() );
        for ( size_t i = 0; i < logicalOutputs.size(); ++i )
        {
            RG_LogicalOutput& logicalOutput = logicalOutputs[i];
            bool mergedResourceFound = false;
            for ( RG_LogicalOutput& mergedOutput : mergedLogicalOutputs )
            {
                if ( mergedOutput.Mergable( logicalOutput ) )
                {
                    mergedOutput.firstTask = std::min( mergedOutput.firstTask, logicalOutput.firstTask );
                    mergedOutput.lastTask = std::max( mergedOutput.lastTask, logicalOutput.lastTask );
                    mergedOutput.name += "_" + logicalOutput.name;
                    logicalOutput.physicalResourceIndex = mergedOutput.physicalResourceIndex;
                    mergedResourceFound = true;
                    break;
                }
            }

            if ( !mergedResourceFound )
            {
                logicalOutput.physicalResourceIndex = static_cast<uint16_t>( mergedLogicalOutputs.size() );
                mergedLogicalOutputs.push_back( logicalOutput );
            }
            resourceNameToIndexMap[logicalOutput.name] = logicalOutput.physicalResourceIndex;
        }

        // 4. Gather all the info needed to create the task objects
        struct ResourceStateTrackingInfo
        {
            ImageLayout currentLayout = ImageLayout::UNDEFINED;
            uint16_t lastWriteTask = USHRT_MAX;
            uint16_t lastReadTask = USHRT_MAX;
            uint8_t lastAttachmentIndex = 255;
        };
        ResourceStateTrackingInfo resourceTrackingInfo[MAX_PHYSICAL_RESOURCES] = {};

        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTask& renderTask = renderTasks[taskIndex];
            RenderTaskBuilder& buildTask = builder.tasks[taskIndex];
            RenderPassDescriptor& renderPassDesc = renderTask.renderPass.desc;

            renderTask.name = std::move( buildTask.name );
            renderTask.renderFunction = buildTask.renderFunction;
            renderTask.renderTargets.numColorAttachments = 0;
            renderTask.renderTargets.depthAttach = USHRT_MAX;
            for ( RG_Element& element : buildTask.elements )
            {
                uint16_t physicalIdx = logicalOutputs[outputNameToLogicalMap[element.name]].physicalResourceIndex;
                ResourceStateTrackingInfo& trackingInfo = resourceTrackingInfo[physicalIdx];
                const RG_LogicalOutput& logicalRes = mergedLogicalOutputs[physicalIdx];

                if ( element.state == ResourceState::WRITE )
                {
                    LoadAction loadOp = LoadAction::LOAD;
                    if ( logicalRes.element.isCleared )
                    {
                        loadOp = LoadAction::CLEAR;
                    }
                    else if ( trackingInfo.currentLayout == ImageLayout::UNDEFINED )
                    {
                        loadOp = LoadAction::DONT_CARE;
                    }

                    if ( element.type == ResourceType::COLOR_ATTACH )
                    {
                        renderTask.renderTargets.colorAttachments[renderTask.renderTargets.numColorAttachments] = physicalIdx;
                        ++renderTask.renderTargets.numColorAttachments;

                        trackingInfo.lastAttachmentIndex = renderPassDesc.numColorAttachments;
                        renderPassDesc.AddColorAttachment( logicalRes.element.format, loadOp, StoreAction::STORE, logicalRes.element.clearColor, trackingInfo.currentLayout, ImageLayout::COLOR_ATTACHMENT_OPTIMAL );
                        trackingInfo.currentLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
                    }
                    else if ( element.type == ResourceType::DEPTH_ATTACH )
                    {
                        renderTask.renderTargets.depthAttach = physicalIdx;

                        renderPassDesc.AddDepthAttachment( logicalRes.element.format, loadOp, StoreAction::STORE, logicalRes.element.clearColor[0], trackingInfo.currentLayout, ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
                        trackingInfo.currentLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    }

                    trackingInfo.lastWriteTask = taskIndex;
                }
                else if ( element.state == ResourceState::READ_ONLY )
                {
                    // if the most recent state was a write, then we need to fixup the last task to change the final layout to READ, for the current task
                    if ( trackingInfo.lastReadTask < trackingInfo.lastWriteTask || (trackingInfo.lastReadTask == USHRT_MAX && trackingInfo.lastWriteTask != USHRT_MAX) )
                    {
                        RenderPassDescriptor& lastDesc = renderTasks[trackingInfo.lastWriteTask].renderPass.desc;
                        if ( logicalRes.element.type == ResourceType::DEPTH_ATTACH )
                        {
                            lastDesc.depthAttachmentDescriptor.finalLayout = ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
                            trackingInfo.currentLayout = ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
                        }
                        else
                        {
                            lastDesc.colorAttachmentDescriptors[trackingInfo.lastAttachmentIndex].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                            trackingInfo.currentLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                        }
                    }

                    trackingInfo.lastReadTask = taskIndex;
                }
            }
        }

        // HACK!
        //renderTasks[numRenderTasks - 1].renderPass.desc.colorAttachmentDescriptors[0].format = VulkanToPGPixelFormat( r_globals.swapchain.GetFormat() );
        //renderTasks[numRenderTasks - 1].renderPass.desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::PRESENT_SRC_KHR;

        // 4. Create the gpu resources. RenderPasses, Textures
        {
            bool error = false;
            numPhysicalResources = static_cast<uint16_t>( mergedLogicalOutputs.size() );
            for ( uint16_t i = 0; i < numPhysicalResources && !error; ++i )
            {
                RG_PhysicalResource& pRes = physicalResources[i];
                const RG_LogicalOutput& lRes = mergedLogicalOutputs[i];
                pRes.name = lRes.name;
                pRes.firstTask = lRes.firstTask;
                pRes.lastTask = lRes.lastTask;

                PG_ASSERT( lRes.element.type != ResourceType::BUFFER );

                TextureDescriptor texDesc;
                texDesc.format = lRes.element.format;
                texDesc.width  = lRes.element.width;
                texDesc.height = lRes.element.height;
                texDesc.depth  = lRes.element.depth;
                texDesc.arrayLayers = lRes.element.arrayLayers;
                texDesc.mipLevels   = lRes.element.mipLevels;
                texDesc.addToBindlessArray = true;
                texDesc.type = ImageType::TYPE_2D;
                texDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                if ( lRes.element.type == ResourceType::COLOR_ATTACH )
                {
                    texDesc.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                }
                else if ( lRes.element.type == ResourceType::DEPTH_ATTACH )
                {
                    texDesc.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                }
            
                pRes.texture = r_globals.device.NewTexture( texDesc, pRes.name );
                error = !pRes.texture;

                stats.memUsedMB += pRes.texture.GetTotalBytes() / (1024.0f * 1024.0f);
                stats.numTextures++;
            }

            for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks && !error; ++taskIdx )
            {
                RenderTask& task = renderTasks[taskIdx];
                task.renderPass = r_globals.device.NewRenderPass( task.renderPass.desc, task.name );

                VkImageView attachments[9];
                uint32_t width = UINT32_MAX;
                uint32_t height = UINT32_MAX;
                for ( uint8_t attachIdx = 0; attachIdx < task.renderTargets.numColorAttachments; ++attachIdx )
                {
                    const Texture& tex = physicalResources[task.renderTargets.colorAttachments[attachIdx]].texture;
                    attachments[attachIdx] = tex.GetView();
                    width = tex.GetWidth();
                    height = tex.GetHeight();
                }
                uint32_t numAttach = task.renderTargets.numColorAttachments;
                if ( task.renderTargets.depthAttach != USHRT_MAX )
                {
                    const Texture& tex = physicalResources[task.renderTargets.depthAttach].texture;
                    attachments[numAttach++] = tex.GetView();
                    width = tex.GetWidth();
                    height = tex.GetHeight();
                }

                VkFramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass      = task.renderPass.GetHandle();
                framebufferInfo.attachmentCount = numAttach;
                framebufferInfo.pAttachments    = attachments;
                framebufferInfo.width           = width;
                framebufferInfo.height          = height;
                framebufferInfo.layers          = 1;
                task.framebuffer = r_globals.device.NewFramebuffer( framebufferInfo, task.name );
                
                error = !task.renderPass || !task.framebuffer;
            }
        }

        return true;
    }


    void RenderGraph::Free()
    {
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            if ( physicalResources[i].texture )
            {
                physicalResources[i].texture.Free();
            }
        }

        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            if ( renderTasks[taskIdx].renderPass )
            {
                renderTasks[taskIdx].renderPass.Free();
            }
            if ( renderTasks[taskIdx].framebuffer )
            {
                renderTasks[taskIdx].framebuffer.Free();
            }
        }
    }


    void RenderGraph::Print() const
    {
        LOG( "Logical Outputs: %u", stats.numLogicalOutputs );
        LOG( "Physical Resources: %u", numPhysicalResources );
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            const auto& res = physicalResources[i];
            LOG( "\tPhysical resource[%u]: '%s'", i, res.name.c_str() );
            LOG( "\t\tUsed in tasks: %u - %u (%s - %s)", res.firstTask, res.lastTask, renderTasks[res.firstTask].name.c_str(), renderTasks[res.lastTask].name.c_str() );
            const auto& tex = res.texture;
            LOG( "\t\tformat: %s, width: %u, height: %u, depth: %u, arrayLayers: %u, mipLevels: %u", PixelFormatName( tex.GetPixelFormat() ).c_str(), tex.GetWidth(), tex.GetHeight(), tex.GetDepth(), tex.GetArrayLayers(), tex.GetMipLevels() );
        }
        LOG( "Physical Resources Mem: %f(MB)", stats.memUsedMB );

        LOG( "Tasks: %u", numRenderTasks );
        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            const auto& task = renderTasks[taskIdx];
            LOG( "Task[%u]: %s", taskIdx, task.name.c_str() );

            uint8_t numColor = 0;
            for ( uint8_t i = 0; i < task.renderPass.desc.numColorAttachments; ++i )
            {
                LOG( "\tColorAttachment[%u]: %s", i, physicalResources[task.renderTargets.colorAttachments[i]].name.c_str() );

                const auto& attach = task.renderPass.desc.colorAttachmentDescriptors[numColor];
                LOG( "\t\tImageLayout: %s -> %s", ImageLayoutToString( attach.initialLayout ).c_str(), ImageLayoutToString( attach.finalLayout ).c_str() );
            }
            if ( task.renderPass.desc.numDepthAttachments != 0 )
            {
                LOG( "\tDepthAttachment: %s", physicalResources[task.renderTargets.depthAttach].name.c_str() );
                const auto& attach = task.renderPass.desc.depthAttachmentDescriptor;
                LOG( "\t\tImageLayout: %s -> %s", ImageLayoutToString( attach.initialLayout ).c_str(), ImageLayoutToString( attach.finalLayout ).c_str() );
            }
        }
    }


    void RenderGraph::Render( Scene* scene, CommandBuffer* cmdBuf, uint32_t swapChainIdx )
    {
        for ( uint16_t i = 0; i < numRenderTasks; ++i )
        {
            RenderTask* task = &renderTasks[i];

            cmdBuf->BeginRenderPass( &task->renderPass, task->framebuffer );
            task->renderFunction( task, scene, cmdBuf );
            cmdBuf->EndRenderPass();
        }

        const Texture& srcTex = GetBackBufferResource()->texture;
        VkImageBlit region;
        memset( &region, 0, sizeof( VkImageBlit ) );
        region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.srcOffsets[1].x = srcTex.GetWidth();
        region.srcOffsets[1].y = srcTex.GetHeight();
        region.srcOffsets[1].z = 1;
        region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.dstOffsets[1].x = r_globals.swapchain.GetWidth();
        region.dstOffsets[1].y = r_globals.swapchain.GetHeight();
        region.dstOffsets[1].z = 1;

        VkImage swapImg = r_globals.swapchain.GetImage( swapChainIdx );
        //cmdBuf->TransitionImageLayout( srcTex.GetHandle(), VK_IMAGE_ASPECT_COLOR_BIT, ImageLayout::COLOR_ATTACHMENT_OPTIMAL, ImageLayout::TRANSFER_SRC_OPTIMAL, PipelineStageFlags::COLOR_ATTACHMENT_OUTPUT_BIT, PipelineStageFlags::TRANSFER_BIT );
        //cmdBuf->TransitionImageLayout( swapImg, VK_IMAGE_ASPECT_COLOR_BIT, ImageLayout::PRESENT_SRC_KHR, ImageLayout::TRANSFER_DST_OPTIMAL, PipelineStageFlags::TOP_OF_PIPE_BIT, PipelineStageFlags::TRANSFER_BIT );
        //cmdBuf->BlitImage( srcTex.GetHandle(), ImageLayout::TRANSFER_SRC_OPTIMAL, swapImg, ImageLayout::TRANSFER_DST_OPTIMAL, region );
        //cmdBuf->TransitionImageLayout( swapImg, VK_IMAGE_ASPECT_COLOR_BIT, ImageLayout::TRANSFER_DST_OPTIMAL, ImageLayout::PRESENT_SRC_KHR, PipelineStageFlags::ALL_COMMANDS_BIT, PipelineStageFlags::ALL_COMMANDS_BIT );

        cmdBuf->TransitionImageLayout( srcTex.GetHandle(), VK_IMAGE_ASPECT_COLOR_BIT, ImageLayout::COLOR_ATTACHMENT_OPTIMAL, ImageLayout::TRANSFER_SRC_OPTIMAL );
        cmdBuf->TransitionImageLayout( swapImg, VK_IMAGE_ASPECT_COLOR_BIT, ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST_OPTIMAL );
        cmdBuf->BlitImage( srcTex.GetHandle(), ImageLayout::TRANSFER_SRC_OPTIMAL, swapImg, ImageLayout::TRANSFER_DST_OPTIMAL, region );
        cmdBuf->TransitionImageLayout( swapImg, VK_IMAGE_ASPECT_COLOR_BIT, ImageLayout::TRANSFER_DST_OPTIMAL, ImageLayout::PRESENT_SRC_KHR );
    }


    RG_PhysicalResource* RenderGraph::GetPhysicalResource( uint16_t idx )
    {
        return &physicalResources[idx];
    }


    RG_PhysicalResource* RenderGraph::GetPhysicalResource( const std::string& logicalName )
    {
        auto it = resourceNameToIndexMap.find( logicalName );
        return it == resourceNameToIndexMap.end() ? nullptr : &physicalResources[it->second];
    }


    RG_PhysicalResource* RenderGraph::GetBackBufferResource()
    {
        return GetPhysicalResource( backBufferName );
    }


    RenderTask* RenderGraph::GetRenderTask( const std::string& name )
    {
        auto it = taskNameToIndexMap.find( name );
        return it == taskNameToIndexMap.end() ? nullptr : &renderTasks[it->second];
    }


    RenderGraph::Statistics RenderGraph::GetStats() const
    {
        return stats;
    }

} // namespace Gfx
} // namespace PG
