#pragma once

namespace PG::Gfx
{
class CommandBuffer;
} // namespace PG::Gfx

namespace PG::UI
{

bool Init();
void Shutdown();

void BootMainMenu();
void BeginFrame();
void EndFrame();
void Render( Gfx::CommandBuffer& cmdBuf );

} // namespace PG::UI
