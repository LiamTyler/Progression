#include "r_taskgraph.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include <unordered_map>


namespace PG
{
namespace Gfx
{

    TG_ResourceDesc::TG_ResourceDesc( ResourceType inType, PixelFormat inFormat, uint32_t inWidth, uint32_t inHeight, uint32_t inDepth, uint32_t inArrayLayers, uint32_t inMipLevels, const glm::vec4& inClearColor, bool inCleared ) :
        type( inType ), format( inFormat ), width( inWidth ), height( inHeight ), depth( inDepth ), arrayLayers( inArrayLayers ), mipLevels( inMipLevels ), clearColor( inClearColor ), isCleared( inCleared )
    {
    }


    void TG_ResourceDesc::ResolveSizes( uint16_t sceneWidth, uint16_t sceneHeight )
    {
        if ( (width & SCENE_WIDTH()) == SCENE_WIDTH() )
        {
            uint32_t divisor = width >> 16;
            width = divisor ? sceneWidth / divisor : sceneWidth;
        }
        if ( (height & SCENE_HEIGHT()) == SCENE_HEIGHT() )
        {
            uint32_t divisor = height >> 16;
            height = divisor ? sceneHeight / divisor : sceneHeight;
        }
        if ( mipLevels == AUTO_FULL_MIP_CHAIN() )
        {
            mipLevels = 1 + static_cast< int >( log2f( static_cast< float >( std::max( width, height ) ) ) );
        }
    }


    bool TG_ResourceDesc::operator==( const TG_ResourceDesc& d ) const
    {
        return format == d.format && width == d.width && height == d.height && depth == d.depth &&
            arrayLayers == d.arrayLayers && mipLevels == d.mipLevels && clearColor == d.clearColor;
    }


    bool TG_ResourceDesc::Mergable( const TG_ResourceDesc& d ) const
    {
        return type == d.type && format == d.format && width == d.width && height == d.height && depth == d.depth && arrayLayers == d.arrayLayers && mipLevels == d.mipLevels;
    }


    RenderTaskBuilder::RenderTaskBuilder( const std::string& inName ) : name( inName )
    {
    }


    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::COLOR_ATTACH, format, width, height, depth, arrayLayers, mipLevels, clearColor, true ) } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::COLOR_ATTACH, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false ) } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, float clearValue )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::DEPTH_ATTACH, format, width, height, 1, 1, 1, glm::vec4( clearValue ), true ) } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::DEPTH_ATTACH, format, width, height, 1, 1, 1, glm::vec4( 0 ), false ) } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::TEXTURE, format, width, height, depth, arrayLayers, mipLevels, clearColor, true ) } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        outputs.push_back( { name, TG_ResourceDesc( ResourceType::TEXTURE, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false ) } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name )        { AddColorOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name )        { AddDepthOutput( name, PixelFormat::INVALID, 0, 0, 0 ); }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name )      { AddTextureOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0, glm::vec4( 0 ) ); }
    void RenderTaskBuilder::AddTextureInput( const std::string& name )       { inputs.push_back( { name } ); }
    void RenderTaskBuilder::AddTextureInputOutput( const std::string& name ) { inputs.push_back( { name } ); }
    void RenderTaskBuilder::SetRenderFunction( RenderFunction func ) { renderFunction = func; }


    RenderGraphBuilder::RenderGraphBuilder()
    {
        tasks.reserve( RenderGraph::MAX_FINAL_TASKS );
    }


    RenderTaskBuilder* RenderGraphBuilder::AddTask( const std::string& name )
    {
        size_t index = tasks.size();
        tasks.push_back( name );
        return &tasks[index];
    }


    GraphResource::GraphResource( const std::string& inName, const TG_ResourceDesc& inDesc, uint16_t currentTask ) :
        name( inName ), desc( inDesc ), firstTask( currentTask ), lastTask( currentTask ), physicalResourceIndex( USHRT_MAX )
    {
    }


    bool GraphResource::Mergable( const GraphResource& res ) const
    {
        bool overlapForward  = res.lastTask >= firstTask && res.firstTask <= lastTask;
        bool overlapBackward = lastTask >= res.firstTask && firstTask <= res.lastTask;
        if ( overlapForward || overlapBackward )
        {
            return false;
        }

        return desc.Mergable( res.desc );
    }


    bool RenderGraph::Compile( RenderGraphBuilder& builder, uint16_t sceneWidth, uint16_t sceneHeight )
    {
        // TODO: re-order tasks for more optimal rendering. For now, keep same order
        // TODO: handle external images
        // TODO: buffers
        // TODO: compute tasks
        // TODO: cull tasks that dont affect the final image
        // TODO: use StoreOp::DONT_CARE when applicable

        assert( builder.tasks.size() <= MAX_FINAL_TASKS );

        numRenderTasks = 0;
        numPhysicalResources = 0;
        stats = {};

        // 1. Create a list of all resources that are (logically) produced by the task graph.
        //    Also resolves scene relatives sizes, and find initial lifetime of resource
        std::vector< GraphResource > graphOutputs;
        std::unordered_map< std::string, uint16_t > outputNameToLogicalMap;

        uint16_t numBuildTasks = static_cast< uint16_t >( builder.tasks.size() );
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = builder.tasks[taskIndex];
            for ( TG_ResourceOutput& output : task.outputs )
            {
                const std::string& name = output.name;
                if ( output.desc.format != PixelFormat::INVALID )
                {
                    if ( outputNameToLogicalMap.find( name ) != outputNameToLogicalMap.end() )
                    {
                        printf( "Duplicate create params for output '%s', only should specify them once\n", name.c_str() );
                        return false;
                    }

                    output.createInfoIdx = static_cast< uint16_t >( graphOutputs.size() );
                    outputNameToLogicalMap[name] = static_cast< uint16_t >( graphOutputs.size() );
                    GraphResource res( name, output.desc, taskIndex );
                    res.desc.ResolveSizes( sceneWidth, sceneHeight );
                    graphOutputs.push_back( res );
                }
                else if ( name == "BACK_BUFFER" )
                {
                    output.createInfoIdx = static_cast< uint16_t >( graphOutputs.size() );
                    outputNameToLogicalMap[name] = static_cast< uint16_t >( graphOutputs.size() );
                    GraphResource res( name, output.desc, taskIndex );
                    graphOutputs.push_back( res );
                }
                else
                {
                    auto it = outputNameToLogicalMap.find( name );
                    if ( it == outputNameToLogicalMap.end() )
                    {
                        printf( "Output '%s' used without any create params specified in this task ('%s') or any previous tasks\n", name.c_str(), task.name.c_str() );
                        return false;
                    }
                    GraphResource& resourceWithCreateInfo = graphOutputs[it->second];
                    if ( resourceWithCreateInfo.firstTask == taskIndex )
                    {
                        printf( "Output '%s' used twice as an output in the same task '%s'\n", name.c_str(), task.name.c_str() );
                        return false;
                    }
                    resourceWithCreateInfo.lastTask = taskIndex;
                    output.createInfoIdx = it->second;
                }
            }
        }

        // 2. Loop over all input resources to finish finding the resource lifetimes.
        //    Currently assuming that all inputs were produced by the output of another task
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = builder.tasks[taskIndex];
            for ( auto& input : task.inputs )
            {
                const std::string& name = input.name;
                auto it = outputNameToLogicalMap.find( name );
                if ( it == outputNameToLogicalMap.end() )
                {
                    printf( "Resource '%s' used as input, but no tasks generate it as an output\n", name.c_str() );
                    return false;
                }

                input.createInfoIdx = it->second;
                GraphResource& res = graphOutputs[it->second];
                if ( res.firstTask >= taskIndex )
                {
                    printf( "Resource '%s' used as input before it created as an output. This means the task ordering is bad\n", name.c_str() );
                    return false;
                }
                res.lastTask = std::max( res.lastTask, taskIndex );
            }
        }

        // 3. Before we create physical resources for these, find out if we can merge two "logical" resources
        //    Two resources are mergeable, if their lifetimes do not overlap at all, and have the same resource descriptors.
        //    Eventually want to allow for different imageviews, or different sized images (ex: storing a 720p image inside of an already allocated 1080p that is no longer needed)
        for ( size_t i = 0; i < graphOutputs.size(); ++i )
        {
            GraphResource& logicalRes = graphOutputs[i];
            for ( uint16_t physicalIdx = 0; physicalIdx < numPhysicalResources; ++physicalIdx )
            {
                GraphResource& physicalRes = physicalResources[physicalIdx];
                if ( physicalRes.Mergable( logicalRes ) )
                {
                    physicalRes.firstTask = std::min( physicalRes.firstTask, logicalRes.firstTask );
                    physicalRes.lastTask = std::max( physicalRes.lastTask, logicalRes.lastTask );
                    logicalRes.physicalResourceIndex = physicalIdx;
                    physicalRes.name += "_" + logicalRes.name;
                    break;
                }
            }

            if ( logicalRes.physicalResourceIndex == USHRT_MAX )
            {
                logicalRes.physicalResourceIndex = numPhysicalResources;
                physicalResources[numPhysicalResources] = logicalRes;
                ++numPhysicalResources;
                assert( numPhysicalResources <= MAX_PHYSICAL_RESOURCES );
            }
        }

        // 4. Gather all the info needed to create the task objects. This includes the RenderPass, and a list of physical resources used
        ImageLayout currentImageLayouts[MAX_PHYSICAL_RESOURCES] = { ImageLayout::UNDEFINED };
        int lastOutputTask[MAX_PHYSICAL_RESOURCES] = { -1 };
        uint8_t lastAttachmentIdx[MAX_PHYSICAL_RESOURCES] = { 0 };
        numRenderTasks = numBuildTasks;
        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTask& renderTask = renderTasks[taskIndex];
            RenderTaskBuilder& buildTask = builder.tasks[taskIndex];
            RenderPassDescriptor& renderPassDesc = renderTask.renderPass.desc;

            renderTask.name = std::move( buildTask.name );
            renderTask.numInputs = 0;
            renderTask.renderFunction = buildTask.renderFunction;
            for ( const TG_ResourceInput& input : buildTask.inputs )
            {
                uint16_t physicalIdx = graphOutputs[input.createInfoIdx].physicalResourceIndex;
                renderTask.inputIndices[renderTask.numInputs] = physicalIdx;
                ++renderTask.numInputs;

                // check to see if the final layout of previous renderpass needs to be adjusted
                if ( currentImageLayouts[physicalIdx] != ImageLayout::UNDEFINED )
                {
                    bool readAndWrite = false;
                    for ( const TG_ResourceOutput& output : buildTask.outputs )
                    {
                        if ( output.name == input.name )
                        {
                            readAndWrite = true;
                            break;
                        }
                    }

                    if ( !readAndWrite )
                    {
                        uint8_t prevTaskIdx = lastOutputTask[physicalIdx];
                        uint8_t attachmentIdx = lastAttachmentIdx[physicalIdx];
                        if ( currentImageLayouts[physicalIdx] == ImageLayout::COLOR_ATTACHMENT_OPTIMAL )
                        {
                            currentImageLayouts[physicalIdx] = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                            // TODO! physicalIdx is not the proper index into the colorAttachmentDescriptors
                            renderTasks[prevTaskIdx].renderPass.desc.colorAttachmentDescriptors[attachmentIdx].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                        }
                        else if ( currentImageLayouts[physicalIdx] == ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
                        {
                            currentImageLayouts[physicalIdx] = ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                            renderTasks[prevTaskIdx].renderPass.desc.depthAttachmentDescriptor.finalLayout = ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                        }
                    }
                }
            }

            renderTask.numOutputs = 0;
            for ( size_t outputIdx = 0; outputIdx < buildTask.outputs.size(); ++outputIdx )
            {
                const GraphResource& resource = graphOutputs[buildTask.outputs[outputIdx].createInfoIdx];
                uint16_t physicalIdx = resource.physicalResourceIndex;
                renderTask.outputIndices[renderTask.numOutputs] = physicalIdx;
                ++renderTask.numOutputs;

                LoadAction loadOp = LoadAction::LOAD;
                if ( resource.desc.isCleared )
                {
                    loadOp = LoadAction::CLEAR;
                }
                else if ( currentImageLayouts[physicalIdx] == ImageLayout::UNDEFINED )
                {
                    loadOp = LoadAction::DONT_CARE;
                }

                // guess at the final layout, fixed up later if needed by the input
                if ( resource.desc.type == ResourceType::COLOR_ATTACH )
                {
                    //buildTask.outputs[outputIdx].renderPassAttachmentIdx = renderPassDesc.numColorAttachments;
                    lastAttachmentIdx[physicalIdx] = renderPassDesc.numColorAttachments;
                    renderPassDesc.AddColorAttachment( resource.desc.format, loadOp, StoreAction::STORE, resource.desc.clearColor, currentImageLayouts[physicalIdx], ImageLayout::COLOR_ATTACHMENT_OPTIMAL );
                    currentImageLayouts[physicalIdx] = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
                }
                else if ( resource.desc.type == ResourceType::DEPTH_ATTACH )
                {
                    renderPassDesc.AddDepthAttachment( resource.desc.format, loadOp, StoreAction::STORE, resource.desc.clearColor[0], currentImageLayouts[physicalIdx], ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
                    currentImageLayouts[physicalIdx] = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                lastOutputTask[physicalIdx] = taskIndex;
            }
        }

        // HACK!
        renderTasks[numRenderTasks - 1].renderPass.desc.colorAttachmentDescriptors[0].format = VulkanToPGPixelFormat( r_globals.swapchain.GetFormat() );
        renderTasks[numRenderTasks - 1].renderPass.desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::PRESENT_SRC_KHR;

        // 5. Create the gpu resources. RenderPasses, Textures
        if ( !AllocatePhysicalResources() )
        {
            return false;
        }

        stats.numLogicalOutputs = static_cast< uint16_t >( graphOutputs.size() );

        return true;
    }


    void RenderGraph::Free()
    {
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            if ( textures[i] )
            {
                textures[i].Free();
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


    void RenderGraph::PrintTaskGraph() const
    {
        printf( "Logical Outputs: %u\n", stats.numLogicalOutputs );
        printf( "Physical Resources: %u\n", numPhysicalResources );
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            const auto& res = physicalResources[i];
            printf( "\tPhysical resource[%u]: '%s'\n", i, res.name.c_str() );
            printf( "\t\tUsed in tasks: %u - %u (%s - %s)\n", res.firstTask, res.lastTask, renderTasks[res.firstTask].name.c_str(), renderTasks[res.lastTask].name.c_str() );
            printf( "\t\tformat: %s, width: %u, height: %u, depth: %u, arrayLayers: %u, mipLevels: %u\n", PixelFormatName( res.desc.format ).c_str(), res.desc.width, res.desc.height, res.desc.depth, res.desc.arrayLayers, res.desc.mipLevels );
        }
        printf( "Physical Resources Mem: %f(MB)\n", stats.memUsedMB );

        printf( "Tasks: %u\n", numRenderTasks );
        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            const auto& task = renderTasks[taskIdx];
            printf( "Task[%u]: %s\n", taskIdx, task.name.c_str() );
            for ( uint8_t i = 0; i < task.numInputs; ++i )
            {
                printf( "\tInput[%u]: %s\n", i, physicalResources[task.inputIndices[i]].name.c_str() );
            }
            uint8_t numColor = 0;
            for ( uint8_t i = 0; i < task.numOutputs; ++i )
            {
                const GraphResource& resource = physicalResources[task.outputIndices[i]];
                printf( "\tOutput[%u]: %s\n", i, physicalResources[task.outputIndices[i]].name.c_str() );

                if ( resource.desc.type == ResourceType::COLOR_ATTACH )
                {
                    const auto& attach = task.renderPass.desc.colorAttachmentDescriptors[numColor];
                    printf( "\t\tImageLayout: %s -> %s\n", ImageLayoutToString( attach.initialLayout ).c_str(), ImageLayoutToString( attach.finalLayout ).c_str() );
                    ++numColor;
                }
                else if ( resource.desc.type == ResourceType::DEPTH_ATTACH )
                {
                    const auto& attach = task.renderPass.desc.depthAttachmentDescriptor;
                    printf( "\t\tImageLayout: %s -> %s\n", ImageLayoutToString( attach.initialLayout ).c_str(), ImageLayoutToString( attach.finalLayout ).c_str() );
                }
            }
        }
    }


    bool RenderGraph::AllocatePhysicalResources()
    {
        bool error = false;
        for ( uint16_t i = 0; i < numPhysicalResources && !error; ++i )
        {
            GraphResource& resource = physicalResources[i];
            if ( resource.name == "BACK_BUFFER" )
            {
                continue;
            }

            TextureDescriptor texDesc;
            texDesc.format = resource.desc.format;
            texDesc.width = resource.desc.width;
            texDesc.height = resource.desc.height;
            texDesc.depth = resource.desc.depth;
            texDesc.arrayLayers = resource.desc.arrayLayers;
            texDesc.mipLevels = resource.desc.mipLevels;
            texDesc.addToBindlessArray = true;
            texDesc.type = ImageType::TYPE_2D;
            texDesc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            if ( resource.desc.type == ResourceType::COLOR_ATTACH )
            {
                texDesc.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            }
            else if ( resource.desc.type == ResourceType::DEPTH_ATTACH )
            {
                texDesc.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            }
            textures[i] = r_globals.device.NewTexture( texDesc, resource.name );
            error = !textures[i];

            stats.memUsedMB += textures[i].GetTotalBytes() / (1024.0f * 1024.0f);
            stats.numTextures++;
        }

        for ( uint16_t i = 0; i < numRenderTasks && !error; ++i )
        {
            RenderTask& task = renderTasks[i];
            task.renderPass = r_globals.device.NewRenderPass( task.renderPass.desc, task.name );
            error = !task.renderPass;
        }

        for ( uint16_t i = 0; i < numRenderTasks && !error; ++i )
        {
            RenderTask& task = renderTasks[i];
            if ( task.name == "postProcessing" )
                continue;
            VkImageView attachments[9];
            VkFramebufferCreateInfo framebufferInfo = {};
            for ( uint8_t outputIdx = 0; outputIdx < task.numOutputs; ++outputIdx )
            {
                const GraphResource& res = physicalResources[task.outputIndices[outputIdx]];
                if ( res.desc.width != physicalResources[task.outputIndices[0]].desc.width || res.desc.height != physicalResources[task.outputIndices[0]].desc.height )
                {
                    printf( "All attachments must be same size. Task %s\n", task.name.c_str() );
                    return false;
                }

                const Texture& tex = textures[task.outputIndices[outputIdx]];
                attachments[outputIdx] = tex.GetView();
            }

            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = task.renderPass.GetHandle();
            framebufferInfo.attachmentCount = task.numOutputs;
            framebufferInfo.pAttachments    = attachments;
            framebufferInfo.width           = physicalResources[task.outputIndices[0]].desc.width;
            framebufferInfo.height          = physicalResources[task.outputIndices[0]].desc.height;
            framebufferInfo.layers          = 1;
            task.framebuffer = r_globals.device.NewFramebuffer( framebufferInfo, task.name );
            error = !task.framebuffer;
        }

        return !error;
    }


    void RenderGraph::Render( Scene* scene, CommandBuffer* cmdBuf )
    {
        for ( uint16_t i = 0; i < numRenderTasks; ++i )
        {
            RenderTask* task = &renderTasks[i];

            if ( task->name == "postProcessing" )
            {
                task->renderFunction( task, scene, cmdBuf );
            }
            else
            {
                cmdBuf->BeginRenderPass( &task->renderPass, task->framebuffer );
                task->renderFunction( task, scene, cmdBuf );
                cmdBuf->EndRenderPass();
            }
        }
    }

} // namespace Gfx
} // namespace PG
