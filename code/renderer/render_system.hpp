#pragma once

#include "graphics_api/pipeline.hpp"
#include <string>

namespace PG
{

class Scene;

namespace RenderSystem
{
bool Init( uint32_t sceneWidth, uint32_t sceneHeight, bool headless = false );
void Shutdown();
void Render();

void CreateTLAS( Scene* scene );
::PG::Gfx::PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name );

} // namespace RenderSystem
} // namespace PG
