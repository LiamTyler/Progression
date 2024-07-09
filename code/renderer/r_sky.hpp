#pragma once

#include "renderer/taskgraph/r_taskGraph.hpp"

namespace PG::Gfx::Sky
{

void Init();
void Shutdown();
void AddTask( TaskGraphBuilder& builder, TGBTextureRef litOutput, TGBTextureRef sceneDepth );

} // namespace PG::Gfx::Sky
