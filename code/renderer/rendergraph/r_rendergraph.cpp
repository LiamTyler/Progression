#include "r_rendergraph.hpp"
#include "renderer/graphics_api/command_buffer.hpp"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include "shared/logger.hpp"
#include <climits>
#include <cstring>
#include <math.h>
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


    void ResolveSizesAndSwapchain( RG_Element& element, const RenderGraphCompileInfo& info )
    {
        element.width = ResolveRelativeSize( info.sceneWidth, info.displayWidth, element.width );
        element.height = ResolveRelativeSize( info.sceneHeight, info.displayHeight, element.height );
        if ( element.mipLevels == AUTO_FULL_MIP_CHAIN() )
        {
            element.mipLevels = 1 + static_cast< int >( log2f( static_cast< float >( std::max( element.width, element.height ) ) ) );
        }
        if ( IsSet( element.type, ResourceType::SWAPCHAIN_IMAGE ) )
            element.format = info.swapchainFormat;
    }


    void RG_TaskRenderTargetsDynamic::AddColorAttach( const RG_Element& element, uint16_t phyRes, LoadAction loadAction, StoreAction storeAction )
    {
        PG_ASSERT( numColorAttach < MAX_COLOR_ATTACHMENTS );
        RG_AttachmentInfo& info = colorAttachInfo[numColorAttach];
        info.clearColor = element.clearColor;
        info.physicalResourceIndex = phyRes;
        info.loadAction = loadAction;
        info.storeAction = storeAction;
        info.format = element.format;
        info.imageLayout = ImageLayout::COLOR_ATTACHMENT;
        ++numColorAttach;
        renderAreaWidth = std::min( renderAreaWidth, (uint16_t)element.width );
        renderAreaHeight = std::min( renderAreaHeight, (uint16_t)element.height );
    }


    void RG_TaskRenderTargetsDynamic::AddDepthAttach( const RG_Element& element, uint16_t phyRes, LoadAction loadAction, StoreAction storeAction )
    {
        PG_ASSERT( depthAttachInfo.format == PixelFormat::INVALID, "Can't add multiple depth targets to a task" );
        depthAttachInfo.clearColor.x = element.clearColor.x;
        depthAttachInfo.physicalResourceIndex = phyRes;
        depthAttachInfo.loadAction = loadAction;
        depthAttachInfo.storeAction = storeAction;
        depthAttachInfo.format = element.format;
        depthAttachInfo.imageLayout = PixelFormatHasStencil( element.format ) ? ImageLayout::DEPTH_STENCIL_ATTACHMENT : ImageLayout::DEPTH_ATTACHMENT;
        renderAreaWidth = std::min( renderAreaWidth, (uint16_t)element.width );
        renderAreaHeight = std::min( renderAreaHeight, (uint16_t)element.height );
    }


    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        ResourceType type = ResourceType::TEXTURE | ResourceType::COLOR_ATTACH;
        elements.push_back( { name, type, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, clearColor, true } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        ResourceType type = ResourceType::TEXTURE | ResourceType::COLOR_ATTACH;
        elements.push_back( { name, type, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::AddSwapChainOutput( const glm::vec4& clearColor )
    {
        ResourceType type = ResourceType::TEXTURE | ResourceType::COLOR_ATTACH | ResourceType::SWAPCHAIN_IMAGE;
        elements.push_back( { "$swapchain", type, ResourceState::WRITE, PixelFormat::INVALID, SIZE_DISPLAY(), SIZE_DISPLAY(), 1, 1, 1, clearColor, true, true } );
    }
    void RenderTaskBuilder::AddSwapChainOutput()
    {
        ResourceType type = ResourceType::TEXTURE | ResourceType::COLOR_ATTACH | ResourceType::SWAPCHAIN_IMAGE;
        elements.push_back( { "$swapchain", type, ResourceState::WRITE, PixelFormat::INVALID, SIZE_DISPLAY(), SIZE_DISPLAY(), 1, 1, 1, glm::vec4( 0 ), false, true } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, float clearValue )
    {
        ResourceType type = ResourceType::TEXTURE | ResourceType::DEPTH_ATTACH;
        if ( PixelFormatHasStencil( format ) )
            type |= ResourceType::STENCIL_ATTACH;
        elements.push_back( { name, type, ResourceState::WRITE, format, width, height, 1, 1, 1, glm::vec4( clearValue ), true } );
    }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height )
    {
        ResourceType type = ResourceType::TEXTURE | ResourceType::DEPTH_ATTACH;
        if ( PixelFormatHasStencil( format ) )
            type |= ResourceType::STENCIL_ATTACH;
        elements.push_back( { name, type, ResourceState::WRITE, format, width, height, 1, 1, 1, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels, const glm::vec4& clearColor )
    {
        elements.push_back( { name, ResourceType::TEXTURE, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, clearColor, true } );
    }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name, PixelFormat format, uint32_t width, uint32_t height, uint32_t depth, uint32_t arrayLayers, uint32_t mipLevels )
    {
        elements.push_back( { name, ResourceType::TEXTURE, ResourceState::WRITE, format, width, height, depth, arrayLayers, mipLevels, glm::vec4( 0 ), false } );
    }
    void RenderTaskBuilder::AddColorOutput( const std::string& name )   { AddColorOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0 ); }
    void RenderTaskBuilder::AddDepthOutput( const std::string& name )   { AddDepthOutput( name, PixelFormat::INVALID, 0, 0 ); }
    void RenderTaskBuilder::AddTextureOutput( const std::string& name ) { AddTextureOutput( name, PixelFormat::INVALID, 0, 0, 0, 0, 0 ); }
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


    bool RenderGraphBuilder::Validate( const RenderGraphCompileInfo& compileInfo ) const
    {
        bool writesToSwapchain = false;
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
            for ( RG_Element element : task.elements )
            {
                ResolveSizesAndSwapchain( element, compileInfo );
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
                        if ( it != logicalOutputs.end() && !element.isExternal )
                        {
                            const std::string& originalTask = tasks[logicalOutputs[name]].name;
                            LOG_ERR( "Output '%s' created for a second time in task %s. Initial creation was in task %s. Should only create an output once", name.c_str(), task.name.c_str(), originalTask.c_str() );
                            return false;
                        }
                        logicalOutputs[name] = it != logicalOutputs.end() ? std::min( it->second, taskIndex ) : taskIndex;
                    }
                    else
                    {
                        if ( it == logicalOutputs.end() )
                        {
                            LOG_ERR( "Output %s in task %s has no create info, but was never created before this task.", name.c_str(), task.name.c_str() );
                            return false;
                        }
                    }

                    numColorAttach += IsSet( element.type, ResourceType::COLOR_ATTACH );
                    numDepthAttach += IsSet( element.type, ResourceType::DEPTH_ATTACH );
                    writesToSwapchain = writesToSwapchain || IsSet( element.type, ResourceType::SWAPCHAIN_IMAGE );
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
                if ( element.state == ResourceState::READ_ONLY && !element.isExternal )
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

        if ( rg.headless && writesToSwapchain )
        {
            LOG_ERR( "Render graph cannot write to swapchain in headless mode. "
                "TODO: automatically substitute custom image for swapchain image while in headless" );
            return false;
        }
        else if ( !rg.headless && !writesToSwapchain )
        {
            LOG_ERR( "At least 1 task must write to the swapchain image while not in headless mode!" );
            return false;
        }

        return true;
    }


    struct RG_LogicalOutput
    {
        RG_LogicalOutput( const RG_Element& inElement, uint16_t taskIdx ) :
            name( inElement.name ), element( inElement ), firstTask( taskIdx ), lastTask( taskIdx ), physicalResourceIndex( USHRT_MAX )
        {}

        bool Mergable( const RG_LogicalOutput& res )
        {
            bool overlapForward  = res.lastTask >= firstTask && res.firstTask <= lastTask;
            bool overlapBackward = lastTask >= res.firstTask && firstTask <= res.lastTask;
            if ( overlapForward || overlapBackward ) return false;
            if ( element.isExternal || res.element.isExternal ) return false;
            if ( !ElementsMergable( this->element, res.element ) ) return false;

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

        assert( builder.tasks.size() <= MAX_TASKS );

        if ( !builder.Validate( compileInfo ) )
        {
            LOG_ERR( "RenderGraph::Compile failed: Invalid RenderGraphBuilder!" );
            return false;
        }

        uint16_t numBuildTasks = static_cast< uint16_t >( builder.tasks.size() );
        numRenderTasks = numBuildTasks;
        numPhysicalResources = 0;
        swapchainPhysicalResIdx = UINT16_MAX;
        stats = {};

        // 1. Create a list of all resources that are (logically) produced by the task graph.
        //    Also resolves scene relatives sizes, and finds the first and last task each resource is used in
        std::vector< RG_LogicalOutput > logicalOutputs;
        std::unordered_map< std::string, uint16_t > outputNameToLogicalMap;

        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTaskBuilder& task = builder.tasks[taskIndex];
            taskNameToIndexMap[task.name] = taskIndex;

            for ( RG_Element& element : task.elements )
            {
                ResolveSizesAndSwapchain( element, compileInfo );

                if ( element.state == ResourceState::WRITE )
                {
                    if ( element.isExternal )
                    {
                        auto it = outputNameToLogicalMap.find( element.name );
                        if ( it == outputNameToLogicalMap.end() )
                        {
                            outputNameToLogicalMap[element.name] = static_cast< uint16_t >( logicalOutputs.size() );
                            logicalOutputs.emplace_back( element, taskIndex );
                        }
                        else
                        {
                            RG_LogicalOutput& logicalOutput = logicalOutputs[it->second];
                            logicalOutput.lastTask = taskIndex;
                        }
                    }
                    else if ( element.format != PixelFormat::INVALID )
                    {
                        outputNameToLogicalMap[element.name] = static_cast< uint16_t >( logicalOutputs.size() );
                        logicalOutputs.emplace_back( element, taskIndex );
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
                if ( IsSet( logicalOutput.element.type, ResourceType::SWAPCHAIN_IMAGE ) )
                    swapchainPhysicalResIdx = logicalOutput.physicalResourceIndex;
            }
            resourceNameToIndexMap[logicalOutput.name] = logicalOutput.physicalResourceIndex;
        }

        // 3. Gather all the info needed to create the task objects
        struct ResourceStateTrackingInfo
        {
            ImageLayout currentLayout = ImageLayout::UNDEFINED;
            uint16_t lastWriteTask = USHRT_MAX;
            uint16_t lastReadTask = USHRT_MAX;
            uint8_t lastAttachmentIndex = 255;
        };

        PG_ASSERT( mergedLogicalOutputs.size() < MAX_PHYSICAL_RESOURCES_PER_FRAME, "Either increase MAX_PHYSICAL_RESOURCES_PER_FRAME or lower resource count" );
        ResourceStateTrackingInfo resourceTrackingInfo[MAX_PHYSICAL_RESOURCES_PER_FRAME] = {};

        for ( uint16_t taskIndex = 0; taskIndex < numBuildTasks; ++taskIndex )
        {
            RenderTask& renderTask = renderTasks[taskIndex];
            RenderTaskBuilder& buildTask = builder.tasks[taskIndex];

            renderTask.name = std::move( buildTask.name );
            renderTask.renderFunction = buildTask.renderFunction;
            RG_TaskRenderTargetsDynamic* rtData = new RG_TaskRenderTargetsDynamic;
            renderTask.renderTargetData = rtData;
            for ( RG_Element& element : buildTask.elements )
            {
                uint16_t physicalIdx = logicalOutputs[outputNameToLogicalMap[element.name]].physicalResourceIndex;
                ResourceStateTrackingInfo& trackingInfo = resourceTrackingInfo[physicalIdx];
                const RG_LogicalOutput& logicalRes = mergedLogicalOutputs[physicalIdx];
                ImageLayout dsLayout = PixelFormatHasStencil( logicalRes.element.format ) ? ImageLayout::DEPTH_STENCIL_ATTACHMENT : ImageLayout::DEPTH_ATTACHMENT;

                if ( element.state == ResourceState::WRITE )
                {
                    LoadAction loadOp = LoadAction::LOAD;
                    if ( element.isCleared )
                        loadOp = LoadAction::CLEAR;
                    else if ( trackingInfo.currentLayout == ImageLayout::UNDEFINED )
                        loadOp = LoadAction::DONT_CARE;

                    ImageLayout previousLayout = trackingInfo.currentLayout;
                    if ( IsSet( element.type, ResourceType::COLOR_ATTACH ) )
                    {
                        rtData->AddColorAttach( logicalRes.element, physicalIdx, loadOp, StoreAction::STORE );
                        trackingInfo.lastAttachmentIndex = rtData->numColorAttach;
                        trackingInfo.currentLayout = ImageLayout::COLOR_ATTACHMENT;
                    }
                    else if ( IsSet( element.type, ResourceType::DEPTH_ATTACH ) )
                    {
                        rtData->AddDepthAttach( logicalRes.element, physicalIdx, loadOp, StoreAction::STORE );
                        trackingInfo.currentLayout = dsLayout;
                    }

                    if ( previousLayout != trackingInfo.currentLayout )
                        renderTask.AddTransition( previousLayout, trackingInfo.currentLayout, physicalIdx, logicalRes.element.type );
                    trackingInfo.lastWriteTask = taskIndex;
                }
                else if ( element.state == ResourceState::READ_ONLY )
                {
                    if ( trackingInfo.lastReadTask < trackingInfo.lastWriteTask || (trackingInfo.lastReadTask == USHRT_MAX && trackingInfo.lastWriteTask != USHRT_MAX) )
                    {
                        const RenderTask& prevTask = renderTasks[trackingInfo.lastWriteTask];
                        ImageLayout desiredLayout = ImageLayout::SHADER_READ_ONLY;
                        if ( IsSet( logicalRes.element.type, ResourceType::DEPTH_ATTACH ) )
                            desiredLayout = dsLayout;

                        renderTask.AddTransition( trackingInfo.currentLayout, desiredLayout, physicalIdx, logicalRes.element.type );
                        trackingInfo.currentLayout = desiredLayout;
                    }
                    
                    trackingInfo.lastReadTask = taskIndex;
                }
            }
        }

        // 4. Create the gpu resources. RenderPasses, Textures
        {
            bool error = false;
            numPhysicalResources = static_cast<uint16_t>( mergedLogicalOutputs.size() );
            for ( uint16_t resIdx = 0; resIdx < numPhysicalResources && !error; ++resIdx )
            {
                const RG_LogicalOutput& lRes = mergedLogicalOutputs[resIdx];

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
                if ( IsSet( lRes.element.type, ResourceType::COLOR_ATTACH ) )
                    texDesc.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                else if ( IsSet( lRes.element.type, ResourceType::DEPTH_ATTACH ) || IsSet( lRes.element.type, ResourceType::STENCIL_ATTACH ) )
                    texDesc.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                if ( lRes.element.isExternal )
                {
                    for ( int frameInFlight = 0; frameInFlight < MAX_FRAMES_IN_FLIGHT; ++frameInFlight )
                    {
                        RG_PhysicalResource& pRes = physicalResources[resIdx][frameInFlight];
                        pRes.name = lRes.name;
                        pRes.firstTask = lRes.firstTask;
                        pRes.lastTask = lRes.lastTask;
                        pRes.texture.m_desc = texDesc;
                        pRes.isExternal = true;
                    }
                    continue;
                }

                for ( int frameInFlight = 0; frameInFlight < MAX_FRAMES_IN_FLIGHT; ++frameInFlight )
                {
                    RG_PhysicalResource& pRes = physicalResources[resIdx][frameInFlight];
                    pRes.name = lRes.name;
                    pRes.firstTask = lRes.firstTask;
                    pRes.lastTask = lRes.lastTask;
                    pRes.texture = rg.device.NewTexture( texDesc, pRes.name );
                    error = !pRes.texture;

                    stats.memUsedMB += pRes.texture.GetTotalBytes() / (1024.0f * 1024.0f);
                    stats.numTextures++;
                }
            }
        }

        return true;
    }


    void RenderGraph::Free()
    {
        for ( int frameInFlight = 0; frameInFlight < MAX_FRAMES_IN_FLIGHT; ++frameInFlight )
        {
            for ( uint16_t i = 0; i < numPhysicalResources; ++i )
            {
                RG_PhysicalResource& res = physicalResources[i][frameInFlight];
                if ( !res.isExternal && res.texture )
                {
                    physicalResources[i][frameInFlight].texture.Free();
                }
            }
        }

        for ( uint16_t taskIndex = 0; taskIndex < numRenderTasks; ++taskIndex )
        {
            if ( renderTasks[taskIndex].renderTargetData )
                delete renderTasks[taskIndex].renderTargetData;
        }
    }


    void RenderGraph::Print() const
    {
        LOG( "Logical Outputs: %u", stats.numLogicalOutputs );
        LOG( "Physical Resources: %u", numPhysicalResources );
        for ( uint16_t i = 0; i < numPhysicalResources; ++i )
        {
            const auto& res = physicalResources[i][0];
            LOG( "\tPhysical resource[%u]: '%s'", i, res.name.c_str() );
            LOG( "\t\tUsed in tasks: %u - %u (%s - %s)", res.firstTask, res.lastTask, renderTasks[res.firstTask].name.c_str(), renderTasks[res.lastTask].name.c_str() );
            const auto& tex = res.texture;
            LOG( "\t\tformat: %s, width: %u, height: %u, depth: %u, arrayLayers: %u, mipLevels: %u", PixelFormatName( tex.GetPixelFormat() ).c_str(), tex.GetWidth(), tex.GetHeight(), tex.GetDepth(), tex.GetArrayLayers(), tex.GetMipLevels() );
        }
        LOG( "Physical Resources Mem: %f(MB)", stats.memUsedMB );

        LOG( "Tasks: %u", numRenderTasks );
        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            const RenderTask& task = renderTasks[taskIdx];
            LOG( "Task[%u]: %s", taskIdx, task.name.c_str() );

            //uint8_t numColor = 0;
            //for ( uint8_t i = 0; i < task.renderTargetData->numColorAttach; ++i )
            //{
            //    LOG( "\tColorAttachment[%u]: %s", i, physicalResources[task.renderTargetData->colorAttachAuxInfo[i].physicalResourceIndex].name.c_str() );
            //
            //    const auto& attach = task.renderPasses[0].desc.colorAttachmentDescriptors[numColor];
            //    LOG( "\t\tImageLayout: %s -> %s", ImageLayoutToString( task.renderTargetData->colorAttachAuxInfo[i].previousLayout ).c_str(), ImageLayoutToString( attach.finalLayout ).c_str() );
            //}
            //if ( task.renderPasses[0].desc.numDepthAttachments != 0 )
            //{
            //    LOG( "\tDepthAttachment: %s", physicalResources[task.renderTargetData->depthAttachAuxInfo.physicalResourceIndex].name.c_str() );
            //    const auto& attach = task.renderPasses[0].desc.depthAttachmentDescriptor;
            //    LOG( "\t\tImageLayout: %s -> %s", ImageLayoutToString( attach.initialLayout ).c_str(), ImageLayoutToString( attach.finalLayout ).c_str() );
            //}
        }
    }


    static void InitVkRenderingAttachmentInfoKHR( VkRenderingAttachmentInfoKHR& info )
    {
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        info.pNext = nullptr;
        info.resolveMode = VK_RESOLVE_MODE_NONE;
        info.resolveImageView = VK_NULL_HANDLE;
        info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }


    void RenderGraph::Render( RG_RenderData& renderData )
    {
        {
            // Kinda hacky, but the swapchain image has a slot in the physical resource array, even if it's
            // not managed by the render graph. Populate it with the data, so accesses between swapchain and non-swapchain
            // images are the same
            Texture& tex = *GetTexture( swapchainPhysicalResIdx );
            tex.m_image = renderData.swapchain->GetImage( renderData.swapchainImageIndex );
            tex.m_imageView = renderData.swapchain->GetImageView( renderData.swapchainImageIndex );
            tex.m_desc.width = renderData.swapchain->GetWidth();
            tex.m_desc.height = renderData.swapchain->GetHeight();
            tex.m_desc.format = renderData.swapchain->GetFormat();
        }
        VkRenderingAttachmentInfoKHR vkColorAttachInfo[MAX_COLOR_ATTACHMENTS];
        for ( int i = 0; i < MAX_COLOR_ATTACHMENTS; ++i )
            InitVkRenderingAttachmentInfoKHR( vkColorAttachInfo[i] );

        VkRenderingAttachmentInfoKHR vkDepthAttachInfo;
        InitVkRenderingAttachmentInfoKHR( vkDepthAttachInfo );

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
		renderInfo.layerCount = 1;

        CommandBuffer& cmdBuf = *renderData.cmdBuf;
        for ( uint16_t taskIdx = 0; taskIdx < numRenderTasks; ++taskIdx )
        {
            RenderTask& task = renderTasks[taskIdx];
            const RG_TaskRenderTargetsDynamic* rtData = task.renderTargetData;
            PG_ASSERT( rtData, "should be true for now, until compute support" );

            PG_DEBUG_MARKER_BEGIN_REGION_CMDBUF( cmdBuf, task.name, DebugMarker::GetNextRegionColor() );
            PG_PROFILE_GPU_START( cmdBuf, task.name );

            for ( const RenderTask::ImageTransition& transition : task.imageTransitions )
            {
                VkImage img = GetTexture( transition.physicalResourceIndex )->GetHandle();
                VkImageSubresourceRange range{ transition.aspect, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
                PipelineStageFlags srcStageMask = GetPipelineStageFlags( transition.previousLayout );
                PipelineStageFlags dstStageMask = GetPipelineStageFlags( transition.desiredLayout );
                cmdBuf.TransitionImageLayout( img, transition.previousLayout, transition.desiredLayout, range, srcStageMask, dstStageMask );
            }

            renderInfo.colorAttachmentCount = rtData->numColorAttach;
            renderInfo.pColorAttachments = rtData->numColorAttach > 0 ? &vkColorAttachInfo[0] : nullptr;
            renderInfo.pDepthAttachment = nullptr;
            renderInfo.pStencilAttachment = nullptr;
			renderInfo.renderArea = { 0, 0, rtData->renderAreaWidth, rtData->renderAreaHeight };

            for ( uint8_t attachIdx = 0; attachIdx < rtData->numColorAttach; ++attachIdx )
            {
                const RG_AttachmentInfo& info = rtData->colorAttachInfo[attachIdx];
                VkRenderingAttachmentInfoKHR& vkAttach = vkColorAttachInfo[attachIdx];
                vkAttach.imageView = GetTexture( info.physicalResourceIndex )->GetView();
                vkAttach.imageLayout = PGToVulkanImageLayout( info.imageLayout );
                vkAttach.loadOp = PGToVulkanLoadAction( info.loadAction );
                vkAttach.storeOp = PGToVulkanStoreAction( info.storeAction );
                memcpy( &vkAttach.clearValue.color.float32[0], &info.clearColor.x, sizeof( glm::vec4 ) );
            }
            if ( rtData->depthAttachInfo.format != PixelFormat::INVALID )
            {
                const RG_AttachmentInfo& info = rtData->depthAttachInfo;
                vkDepthAttachInfo.imageView = GetTexture( info.physicalResourceIndex )->GetView();
                vkDepthAttachInfo.imageLayout = PGToVulkanImageLayout( info.imageLayout );
                vkDepthAttachInfo.loadOp = PGToVulkanLoadAction( info.loadAction );
                vkDepthAttachInfo.storeOp = PGToVulkanStoreAction( info.storeAction );
                vkDepthAttachInfo.clearValue.color.float32[0] = info.clearColor.x;

                renderInfo.pDepthAttachment = &vkDepthAttachInfo;
                if ( PixelFormatHasStencil( rtData->depthAttachInfo.format ) )
                    renderInfo.pStencilAttachment = &vkDepthAttachInfo;
            }

            vkCmdBeginRendering( cmdBuf.GetHandle(), &renderInfo );
            task.renderFunction( &task, renderData );
            vkCmdEndRendering( cmdBuf.GetHandle() );

            PG_PROFILE_GPU_END( cmdBuf, task.name );
            PG_DEBUG_MARKER_END_REGION_CMDBUF( cmdBuf );
        }

        // transition the swapchain image to PRESENT
        VkImage img = GetTexture( swapchainPhysicalResIdx )->GetHandle();
        VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS };
        PipelineStageFlags srcStageMask = GetPipelineStageFlags( ImageLayout::COLOR_ATTACHMENT );
        PipelineStageFlags dstStageMask = GetPipelineStageFlags( ImageLayout::PRESENT_SRC_KHR );
        cmdBuf.TransitionImageLayout( img, ImageLayout::COLOR_ATTACHMENT, ImageLayout::PRESENT_SRC_KHR, range, srcStageMask, dstStageMask );
        
        frameInFlight = (frameInFlight + 1) % MAX_FRAMES_IN_FLIGHT;
    }


    Texture* RenderGraph::GetTexture( uint16_t idx )
    {
        return &physicalResources[idx][frameInFlight].texture;
    }

    RG_PhysicalResource* RenderGraph::GetPhysicalResource( uint16_t idx )
    {
        return &physicalResources[idx][frameInFlight];
    }

    RG_PhysicalResource* RenderGraph::GetPhysicalResource( const std::string& logicalName, uint8_t frameIdx )
    {
        auto it = resourceNameToIndexMap.find( logicalName );
        return it == resourceNameToIndexMap.end() ? nullptr : &physicalResources[it->second][frameIdx];
    }


    RenderTask* RenderGraph::GetRenderTask( const std::string& name )
    {
        auto it = taskNameToIndexMap.find( name );
        return it == taskNameToIndexMap.end() ? nullptr : &renderTasks[it->second];
    }


    PipelineAttachmentInfo RenderGraph::GetPipelineAttachmentInfo( const std::string& name ) const
    {
        PipelineAttachmentInfo info{};
        auto it = taskNameToIndexMap.find( name );
        if ( it != taskNameToIndexMap.end() )
        {
            const RenderTask& task = renderTasks[it->second];
            if ( !task.renderTargetData )
                return info;

            info.numColorAttachments = task.renderTargetData->numColorAttach;
            for ( uint8_t i = 0; i < task.renderTargetData->numColorAttach; ++i )
                info.colorAttachmentFormats[i] = task.renderTargetData->colorAttachInfo[i].format;
            info.depthAttachmentFormat = task.renderTargetData->depthAttachInfo.format;
        }

        return info;
    }


    RenderGraph::Statistics RenderGraph::GetStats() const
    {
        return stats;
    }

} // namespace Gfx
} // namespace PG
