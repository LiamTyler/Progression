#pragma once

#include "renderer/taskgraph/r_taskGraph.hpp"

namespace PG::Gfx::Lighting
{

void Init();
void Shutdown();

void UpdateLights( Scene* scene );
void AddShadowTasks( Scene* scene, TaskGraphBuilder& builder );

u32 GetLightCount();
u64 GetLightBufferAddress();

} // namespace PG::Gfx::Lighting
