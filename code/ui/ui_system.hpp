#pragma once

namespace PG::Gfx
{
class CommandBuffer;
} // namespace PG::Gfx

namespace PG::UI
{

bool Init();
void Shutdown();

void Update();
void Render( Gfx::CommandBuffer& cmdBuf );

} // namespace PG::UI
