#pragma once
#pragma once

#include "graphics_api/pipeline.hpp"
#include <string>

namespace PG
{

class Scene;

namespace RenderSystem
{
bool Init( uint32_t sceneWidth, uint32_t sceneHeight, uint32_t displayWidth, uint32_t displayHeight, bool headless = false );
void Resize( uint32_t displayWidth, uint32_t displayHeight );
void Shutdown();
void Render();

void CreateTLAS( Scene* scene );
::PG::Gfx::PipelineAttachmentInfo GetPipelineAttachmentInfo( const std::string& name );

} // namespace RenderSystem
} // namespace PG
